# Localization Files

This directory contains localization files for OBS Polyemesis in multiple languages.

## Translation Status

| Language | Code | Status | Translator |
|----------|------|--------|------------|
| English (US) | en-US | ✅ Complete | Official |
| Japanese | ja-JP | ✅ Complete | AI Translation |
| Korean | ko-KR | ✅ Complete | AI Translation |
| Simplified Chinese | zh-CN | ✅ Complete | AI Translation |
| Traditional Chinese | zh-TW | ✅ Complete | AI Translation |
| Spanish | es-ES | ✅ Complete | AI Translation |
| Brazilian Portuguese | pt-BR | ✅ Complete | AI Translation |
| German | de-DE | ✅ Complete | AI Translation |
| French | fr-FR | ✅ Complete | AI Translation |
| Russian | ru-RU | ✅ Complete | AI Translation |
| Italian | it-IT | ✅ Complete | AI Translation |

**Note:** All translations are AI-generated and may benefit from native speaker review. Community contributions to refine translations are welcome!

## Contributing Translations

### Quick Start

1. **Choose a language file** from the list above
2. **Open the `.ini` file** (e.g., `ja-JP.ini` for Japanese)
3. **Translate the values** (keep the keys unchanged)
4. **Remove the "NEEDS TRANSLATION" comment** at the top
5. **Submit a pull request** with your translations

### Translation Guidelines

#### DO:
- ✅ Translate only the **values** (text inside quotes)
- ✅ Keep the **keys** unchanged (e.g., `PluginName=`, `Start=`)
- ✅ Maintain proper encoding (UTF-8)
- ✅ Test your translations in OBS Studio
- ✅ Use natural, native language expressions
- ✅ Keep translations concise for UI elements

#### DON'T:
- ❌ Translate service names (Twitch, YouTube, Facebook, etc.)
- ❌ Translate technical terms (RTMP, HTTPS, URL, etc.)
- ❌ Change the file encoding
- ❌ Modify the key names (left side of `=`)
- ❌ Add or remove lines

### Example Translation

**Before (Template):**
```ini
# Translation Status: NEEDS TRANSLATION - This is a template file
Start="Start"
Stop="Stop"
Refresh="Refresh"
```

**After (Translated to Spanish):**
```ini
# Spanish (Español) Localization
Start="Iniciar"
Stop="Detener"
Refresh="Actualizar"
```

### File Format

OBS uses `.ini` file format for localizations:

```ini
# Comments start with #
KeyName="Translated Value"
```

### Testing Your Translation

1. Copy your translated `.ini` file to your OBS locale folder:
   - **Windows**: `%AppData%\obs-studio\obs-plugins\obs-polyemesis\locale\`
   - **macOS**: `~/Library/Application Support/obs-studio/obs-plugins/obs-polyemesis/locale/`
   - **Linux**: `~/.config/obs-studio/obs-plugins/obs-polyemesis/locale/`

2. Change OBS Studio's language in Settings → General → Language

3. Restart OBS Studio and verify your translations appear correctly

## Translation Tools

### Recommended Tools:
- **Poedit** - Free translation editor with .ini support
- **VS Code** - With "ini for VSCode" extension
- **Text Editor** - Any UTF-8 capable editor

### Online Resources:
- [OBS Studio Translation Guide](https://github.com/obsproject/obs-studio/wiki/Translation-Guide)
- [OBS Locale Codes](https://crowdin.com/project/obs-studio)

## Language Priority

### Tier 1 (Essential - Highest Impact)
Languages covering ~80% of global streaming community:
- 🇯🇵 Japanese (ja-JP)
- 🇰🇷 Korean (ko-KR)
- 🇨🇳 Simplified Chinese (zh-CN)
- 🇹🇼 Traditional Chinese (zh-TW)
- 🇪🇸 Spanish (es-ES)
- 🇧🇷 Brazilian Portuguese (pt-BR)
- 🇩🇪 German (de-DE)
- 🇫🇷 French (fr-FR)
- 🇷🇺 Russian (ru-RU)
- 🇮🇹 Italian (it-IT)

### Tier 2 (Important - Next Priority)
If you'd like to contribute translations for other languages, please open an issue first to discuss adding the locale file.

## String Count

Each locale file contains approximately:
- **90 translatable strings**
- **8 sections**: Plugin Info, Source Plugin, Output Plugin, Dock Panel, Process Control, Process Information, Multistream, Messages

## Questions?

If you have questions about translating OBS Polyemesis:
1. Open an issue on GitHub
2. Check the [Contributing Guide](../../CONTRIBUTING.md)
3. Review existing translations (en-US.ini) for context

## Credits

Translations are contributed by the community. Contributors will be credited in:
- This README.md (Translator column)
- Plugin credits
- Release notes

---

**Thank you for helping make OBS Polyemesis accessible to users worldwide!** 🌍
