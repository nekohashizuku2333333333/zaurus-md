# Markdown association repair (0.5)

The system MIME recovery in 0.2 did not restore missing zaurusmd desktop
entries. Recovery now checks for the installed executable and restores both
package-owned entries, claiming text/markdown and text/x-markdown. Without
an executable, recovery reports that installation is required, rather than
creating a nonfunctional launcher.

The launchers now use Exec=zaurusmd, without the freedesktop %f placeholder.
In the Qtopia 1.7 SDK source, Global::execute sends the document separately;
AppLauncher::execute splits the command and appends the document. A literal
%f therefore becomes argv[1], ahead of the actual document on cold launch.
The editor already uses showMainDocumentWidget and setDocument for Qtopia IPC.

On the device, run as root in the directory containing the downloaded file:

```sh
sh restore-ipk-association.sh
```

If the editor is absent, install instead:

```sh
ipkg install zaurusmd_0.5_arm.ipk
```

Both paths request a Qtopia link refresh. Close and reopen the file manager;
if its cached association persists, save other work before restarting Qtopia.
Do not delete system MIME files or reset unrelated default applications.

Validation: 57 isolated package/association checks and legacy SDK ipkg
install/remove/reinstall/direct-upgrade tests passed. These validate packaging
and recovery, not the physical device's running Sharp file manager. The exact
live cause of its undefined text/markdown message still requires device-side
confirmation after repair. The application binary is unchanged from 0.2.
