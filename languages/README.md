# Skea Clock Language Editor

Open `language-editor.html` in a modern browser. It works offline as a single file; no installation, server, or developer tools are needed.

## Create a language

1. Use **KOR / ENG** at the top to choose the editor's interface language. Reference translations and your input stay unchanged.
2. Enter a **Display name** for the app's Language menu and a **Language code**, such as `fr` or `pt-BR` (up to 15 characters).
3. Translate the phrases beside the built-in Korean source. Hover over a field or place the cursor in it to highlight where the phrase appears in the menu or dialog preview.
4. Click **Download language file** to save your `.h` file.
5. In Skea Clock, right-click the clock and choose **Language → Choose language file…** (`언어 파일 선택…` in Korean), then select the downloaded file. No rebuild is needed.

## Edit and compare

- Upload or drag and drop one or more `.h` language files onto the page.
- Choose **Edit language file** to replace the editor's display name, code, and translation inputs with that file's contents.
- Choose **Add as reference** to show its translations alongside Korean without replacing your input. Remove a reference with its **×** button; the built-in Korean reference always remains.
- The preview updates as you type. Alarm times, filenames, and display details are examples.

## Saving and untranslated phrases

- Blank translations are saved as blank. The app and preview use Korean for empty or missing phrases, including phrases added by later app updates.
- Each weekday label can contain up to three characters. Blank weekday labels also fall back to Korean.
- The editor preserves file filter patterns; translate only their display names. Keep any trailing spaces indicated beside a field.
- Inputs are saved locally in your browser when browser storage is available. Download your file to keep or share a copy; uploaded reference files are not saved in the draft.
- Skea Clock remembers one custom language file's path. Keep the file at that location: moving, deleting, or invalidating it causes a warning and removes its menu entry. Selecting another valid custom file replaces the previous one.

