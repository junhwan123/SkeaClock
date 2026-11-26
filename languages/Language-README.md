# Skea Clock Language Editor

Download [language-editor.html](language-editor.html) and open it in your browser to create a Skea Clock language file. The editor runs offline as a single HTML file with the original Korean text built in.

## Create a language

1. Use **KOR / ENG** at the top to choose the editor's interface language.
2. Enter a **Display name** for the app's Language menu and a **Language code**, such as `fr` or `pt-BR` (up to 15 characters).
3. Enter your translations beside the Korean source.

## Edit and compare

- Upload or drag and drop one or more `.h` language files onto the page.
- Choose **Edit language file** to replace the editor's display name, code, and translation inputs with that file's contents.
- Choose **Add as reference** to show another file's translations alongside the Korean source and your own input. You can add multiple reference languages and remove them with **×**.

## Preview your translation

Hover over or select a translation field to open the corresponding menu or dialog and highlight the phrase. The preview updates as you type and shows how your translation will appear in the app. It uses the Korean source for blank fields. Alarm times, filenames, and display details are examples.

## Save and apply

1. Click **Download language file** to save your translation as a `.h` file.
2. In Skea Clock, right-click the clock and choose **Language → Choose language file...** (**언어 파일 선택...** in Korean). Select your downloaded file to apply it.

- Blank translations are saved as blank. The app uses Korean for empty or missing phrases, including phrases added by later app updates.
- Each weekday label can contain up to three characters. Blank weekday labels also fall back to Korean.
- The browser saves your draft when local storage is available. Download the file to keep or share a copy.
- Skea Clock remembers the path of one custom language file. Keep the file at that location so the app can load it again. Selecting another valid custom file replaces the previous one.

