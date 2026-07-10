/**
 * @file CsvLoggerController.cpp
 * @brief Implementazione del logger CSV produttore/consumatore.
 */
#include "controller/CsvLoggerController.h"

#include <iomanip>

namespace am {

CsvLoggerController::~CsvLoggerController()
{
    stop();
}

bool CsvLoggerController::start(const wxString& path)
{
    stop();  // chiude un'eventuale registrazione precedente ancora attiva

    file_.open(std::string(path.utf8_string()), std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        return false;
    }

    file_ << "tempo_s";
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        file_ << ",A" << ch << "_V";
    }
    file_ << '\n';

    currentPath_ = path;
    stopping_ = false;
    active_.store(true);
    thread_ = std::thread(&CsvLoggerController::writerMain, this);
    return true;
}

void CsvLoggerController::stop()
{
    if (!active_.exchange(false)) {
        return;  // già ferma: idempotente
    }
    {
        const std::lock_guard lock(queueMutex_);
        stopping_ = true;
    }
    queueCv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();  // il consumatore drena la coda prima di uscire (vedi writerMain)
    }
    file_.close();
    currentPath_.clear();

    // Scarta eventuali campioni accodati in race con questo stop(): push() legge
    // active_ fuori dal lock per restare non bloccante, quindi in teoria un
    // push() concorrente può accodare una riga DOPO che il consumatore è già
    // uscito (join() sopra già completato). Senza questa pulizia quella riga
    // "fantasma" (timestamp della sessione appena chiusa) resterebbe in coda e
    // diventerebbe la prima riga scritta dalla PROSSIMA registrazione, con un
    // tempo_s incoerente. Il campione perso qui è al più uno, esattamente nella
    // finestra di poche istruzioni tra la chiusura e questo punto: accettabile,
    // il contrario (farlo sopravvivere alla sessione successiva) non lo è.
    const std::lock_guard lock(queueMutex_);
    queue_.clear();
}

void CsvLoggerController::push(double t,
                               const std::array<std::uint16_t, kNumAnalogChannels>& values)
{
    if (!active_.load(std::memory_order_relaxed)) {
        return;  // nessuna registrazione in corso: niente da fare
    }
    {
        const std::lock_guard lock(queueMutex_);
        queue_.push_back(Row{ t, values });
    }
    queueCv_.notify_one();
}

void CsvLoggerController::writerMain()
{
    for (;;) {
        std::deque<Row> batch;
        bool shouldStop = false;
        {
            std::unique_lock lock(queueMutex_);
            queueCv_.wait(lock, [this] { return !queue_.empty() || stopping_; });
            batch.swap(queue_);
            shouldStop = stopping_;
        }
        for (const auto& row : batch) {
            writeRow(row);
        }
        if (batch.empty() && shouldStop) {
            break;  // coda vuota e chiusura richiesta: tutto scritto, si esce.
        }
    }
    file_.flush();
}

void CsvLoggerController::writeRow(const Row& row)
{
    file_ << std::fixed << std::setprecision(6) << row.t;
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const double v = static_cast<double>(row.raw[static_cast<std::size_t>(ch)])
                        * kAdcReferenceVolt / kAdcMaxValue;
        file_ << ',' << std::setprecision(4) << v;
    }
    file_ << '\n';
}

} // namespace am
