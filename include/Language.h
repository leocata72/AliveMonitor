/**
 * @file Language.h
 * @brief Lingue dell'interfaccia supportate.
 */
#pragma once

namespace am {

enum class Language {
    IT,  ///< Italiano (lingua originale del progetto).
    EN,  ///< Inglese (internazionale).
    FR,  ///< Francese.
    ES,  ///< Spagnolo.
    DE,  ///< Tedesco.
};

/// Numero di lingue supportate: usato per dimensionare le tabelle di
/// traduzione e per validare l'indice letto da AppSettings.
inline constexpr int kLanguageCount = 5;

/// Lingua predefinita se il file di configurazione manca o è illeggibile.
inline constexpr Language kDefaultLanguage = Language::IT;

} // namespace am
