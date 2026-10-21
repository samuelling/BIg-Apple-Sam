# Signing the screen savers

The build (`.github/workflows/screensaver.yml`) signs the screen savers by itself once these secrets exist. Until then the Mac build is unsigned and people see the "can't be opened" warning.

Add secrets and variables under the repo's **Settings → Secrets and variables → Actions**, or with `gh secret set NAME` / `gh variable set NAME`.

## macOS: Developer ID + notarization (included in the Apple Developer Program)

You need a **Developer ID Application** certificate. This is not the Apple Development or Apple Distribution certificate you use for TestFlight.

1. In Xcode, go to **Settings → Accounts**, select your team, then **Manage Certificates… → + → Developer ID Application**.
2. In Keychain Access → **My Certificates**, right-click **Developer ID Application: Sam Ling (…)** and choose **Export**. Save it as `cert.p12` with a password.
3. Add these two secrets:
   ```sh
   base64 -i cert.p12 | gh secret set MACOS_CERT_P12
   gh secret set MACOS_CERT_PASSWORD        # paste the export password
   ```
4. In App Store Connect → **Users and Access → Integrations → App Store Connect API → Team Keys**, generate a key with **Developer** access. Download the `.p8` file (you can only download it once) and note its **Key ID** and the **Issuer ID** shown above the list.
5. Add these three secrets:
   ```sh
   gh secret set NOTARY_KEY_P8 < AuthKey_XXXXXXXXXX.p8
   gh secret set NOTARY_KEY_ID              # e.g. XXXXXXXXXX
   gh secret set NOTARY_ISSUER_ID           # a UUID
   ```
6. Delete `cert.p12` from your Downloads folder.

On every push, the build signs `Big Apple Sam.saver` with the hardened runtime and a timestamp. It then sends it to Apple's notary service, staples the ticket so it opens even offline, and does the same for the `.dmg`.

## Windows

The Windows build stays unsigned: code signing there costs money (about $10 a month through Azure Artifact Signing), and even signed builds get SmartScreen warnings until they build up a download history. People click **More info → Run anyway** once.
