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

bool CsvLoggerController::start(const wxString& path,
                                const ChannelCalibrations& calibrations)
{
    stop();  // chiude un'eventuale registrazione precedente ancora attiva

    file_.open(std::string(path.utf8_string()), std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        return false;
    }

    calibrations_ = calibrations;  // istantanea: congelata per l'intera sessione

    file_ << "tempo_s";
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        file_ << ",A" << ch << "_V";
    }
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        // Unità incorporata nel nome colonna (es. A0_conv[°C]): un CSV letto
        // in un foglio elettronico resta autoesplicativo senza righe extra
        // di intestazione da interpretare.
        file_ << ",A" << ch << "_conv["
              << calibrations_[static_cast<std::size_t>(ch)].unit << ']';
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

    // Pulizia difensiva: con il controllo active_/stopping_ ora spostato
    // DENTRO queueMutex_ in push() (vedi sotto), il consumatore drena sempre
    // la coda per intero prima di uscire da writerMain() (l'ultima
    // iterazione vede stopping_==true E queue_ vuoto, altrimenti continua),
    // quindi a questo punto queue_ dovrebbe già essere vuota. Lasciato come
    // rete di sicurezza a costo pressoché nullo: uno svuotamento di una coda
    // già vuota è un'operazione a costo nullo.
    const std::lock_guard lock(queueMutex_);
    queue_.clear();
}

void CsvLoggerController::push(double t,
                               const std::array<std::uint16_t, kNumAnalogChannels>& values)
{
    // NB: il controllo di active_/stopping_ deve avvenire SOTTO lo stesso
    // lock che protegge la push_back, non prima di acquisire il lock come
    // nella versione precedente (che leggeva active_ fuori dal mutex). Con
    // quel pattern "check poi lock" un push() poteva essere sospeso fra il
    // controllo e l'acquisizione del lock abbastanza a lungo da far
    // completare per intero uno stop() concorrente (drenaggio della coda,
    // join del consumatore, pulizia finale): al risveglio il push() accodava
    // comunque la riga, ma ormai nessuno restava a consumarla ("riga
    // fantasma"), che sarebbe finita come prima riga della sessione CSV
    // SUCCESSIVA con un tempo_s incoerente. Controllando active_/stopping_ e
    // scrivendo in coda sotto lo stesso lock, le due cose sono atomiche
    // rispetto a stop(): se stopping_ è già true (o active_ già false) qui
    // dentro, il campione viene scartato invece di essere accodato a un
    // consumatore che potrebbe già essere uscito.
    const std::lock_guard lock(queueMutex_);
    if (stopping_ || !active_.load(std::memory_order_relaxed)) {
        return;  // nessuna registrazione in corso o in chiusura: niente da fare
    }
    queue_.push_back(Row{ t, values });
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
    std::array<double, kNumAnalogChannels> volts{};
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        volts[chIdx] = static_cast<double>(row.raw[chIdx])
                      * kAdcReferenceVolt / kAdcMaxValue;
        file_ << ',' << std::setprecision(4) << volts[chIdx];
    }
    for (int ch = 0; ch < kNumAnalogChannels; ++ch) {
        const auto chIdx = static_cast<std::size_t>(ch);
        const double converted = calibrations_[chIdx].convert(volts[chIdx]);
        file_ << ',' << std::setprecision(4) << converted;
    }
    file_ << '\n';
}

} // namespace am
