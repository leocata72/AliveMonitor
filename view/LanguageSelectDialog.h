/**
 * @file LanguageSelectDialog.h
 * @brief Dialogo di scelta della lingua, mostrato una sola volta al primo
 *        avvio (nessuna preferenza ancora salvata su disco).
 *
 * VIEW (MVC). Caso particolare rispetto alle altre View: a questo punto
 * dell'avvio la lingua dell'interfaccia non è ancora nota (è proprio quello
 * che questo dialogo deve determinare), quindi il suo testo non può passare
 * da tr(StringId) come il resto dell'applicazione — sarebbe una dipendenza
 * circolare. Il prompt è mostrato per esteso nelle 5 lingue supportate
 * contemporaneamente; solo i nomi nelle wxChoice (nativeLanguageName) e il
 * pulsante OK, entrambi già indipendenti dalla lingua corrente, sono
 * riutilizzati così come sono.
 */
#pragma once

#include <wx/dialog.h>

#include "Language.h"

class wxChoice;

namespace am {

class LanguageSelectDialog : public wxDialog {
public:
    explicit LanguageSelectDialog(wxWindow* parent);

    /// Lingua correntemente selezionata nel wxChoice. Va letta a prescindere
    /// dal codice di ritorno di ShowModal(): il dialogo ha solo il pulsante
    /// OK (nessun Cancel ha senso qui, la scelta è obbligatoria), quindi
    /// anche una chiusura via [X] restituisce comunque la selezione corrente
    /// (il default preselezionato, se l'utente non ha toccato nulla).
    [[nodiscard]] Language selectedLanguage() const;

private:
    wxChoice* languageChoice_ = nullptr;
};

} // namespace am
