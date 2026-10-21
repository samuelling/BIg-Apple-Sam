# Signing the screen savers

The build (`.github/workflows/screensaver.yml`) signs the screen savers by itself once these secrets exist. Until then it ships unsigned builds, and people see the "can't be opened" (Mac) or "Windows protected your PC" warnings.

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

## Windows: Azure Artifact Signing (about $9.99 a month)

This is open to individual developers in the US and Canada.

1. In the Azure portal, under the subscription, register the **Microsoft.CodeSigning** resource provider. Then create an **Artifact Signing account** on the Basic tier and note its region. The endpoint for East US is `https://eus.codesigning.azure.net/`.
2. In the account, start **Identity validation → Individual**. This involves an ID check and can take a few days.
3. Once you're validated, create a **certificate profile** of type **Public Trust**.
4. In Microsoft Entra ID → **App registrations**, create an app called `big-apple-sam-signing`.
5. Under **Certificates & secrets → Federated credentials**, add one for **GitHub Actions**: organization `samuelling`, repository `Big-Apple-Sam`, entity **Branch** `main`.
6. On the Artifact Signing account, under **Access control (IAM)**, give that app the **Artifact Signing Certificate Profile Signer** role.
7. Add these secrets and variables:
   ```sh
   gh secret set AZURE_CLIENT_ID            # the app's Application (client) ID
   gh secret set AZURE_TENANT_ID            # Directory (tenant) ID
   gh secret set AZURE_SUBSCRIPTION_ID
   gh variable set AZURE_SIGNING_ENDPOINT --body https://eus.codesigning.azure.net/
   gh variable set AZURE_SIGNING_ACCOUNT --body <account name>
   gh variable set AZURE_CERT_PROFILE --body <profile name>
   ```

The build signs `BigAppleSam.scr` and the installer with a timestamp.

Signing doesn't stop SmartScreen warnings straight away. Reputation builds up as people download the files, and since 2024 even EV certificates don't skip this. Signing every release with the same identity lets each new build inherit the reputation of the ones before.
