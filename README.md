# Super IRCam

Qt 6 desktop viewer for the Qianli Super IRCam Pro 2S (and other InfiRay-family
USB modules that appear as UVC `0bda:5830`).

Inspired by [Thermal-Camera-Redux](https://github.com/92es/Thermal-Camera-Redux),
but developed as a new application from scratch. Super IRCam does not fork,
link, exec, or vendor Redux source. It is an independent C++ program.

## Hardware

Qianli Super IRCam Pro 2S (MESA-IDEA stand) over a PCB:

![Qianli Super IRCam Pro 2S on a stand above a PCB](docs/images/hardware_setup.png)

## Screenshots

Default Jet view with on-image min / max / center temperatures and the
two-column control panel:

![Super IRCam Jet colormap](docs/images/gui_jet.png)

Inferno, Ironbow, and Magma on the same scene:

![Inferno colormap](docs/images/gui_inferno.png)

![Ironbow colormap](docs/images/gui_ironbow.png)

![Magma colormap](docs/images/gui_magma.png)

Image + thermal side-by-side layout:

![Img+Therm wide layout](docs/images/gui_wide.png)

Visual-only layout (camera Y plane):

![Image layout](docs/images/gui_visual.png)

## Redux vs Super IRCam

**Thermal-Camera-Redux** is a keyboard-and-OSD Linux app built around OpenCV
HighGUI (`src/tc001.cpp`). It opens the camera with OpenCV, draws menus on the
video window, and is launched as the `redux` command.

**Super IRCam** is a separate codebase written from scratch with:

- raw **V4L2** capture (mmap YUYV 256×384)
- **Qt 6** widgets for the button GUI
- **OpenCV** only for colormap, resize, PNG, and AVI

Both talk to the same USB radiometry format (lower 256×192 of the 256×384
YUYV frame holds 16-bit Kelvin; `T°C = raw/64 − 273.15`) and Super IRCam
reimplements the Redux colormap set and overlay behaviour. Installing
`thermal-camera-redux` is not required for `super-ircam` to run.

## License

The thermal viewer lineage and its prior licenses belong to
[Thermal-Camera-Redux](https://github.com/92es/Thermal-Camera-Redux) and
[PyThermalCamera](https://github.com/leswright1977/PyThermalCamera). We did
not relicense that work. This repository is original Qt GUI source plus portable
packaging. See [LICENSE](LICENSE).

Related Redux packaging fork (CLI `redux`, Docker `.deb` build, desktop-launch
stdin fix):

https://github.com/stulluk/Thermal-Camera-Redux

## Features

- Auto-select USB `0bda:5830` (does not open a normal webcam)
- 37 Redux colormaps, 7 interpolations, 4 layouts
- On-image max / min / center crosshairs with °C (or °F)
- Up to 12 user spots (left-click add, right-click clear)
- Snapshot PNG+RAW and AVI recording (MJPG)
- Save Settings to `~/.config/super-ircam/settings.json`
- Start-menu and desktop launcher icons

## Install

There is no `.deb`. Every push to `main` builds two portable packages on
GitHub Actions (`.github/workflows/packages.yml`):

- **AppImage** — `SuperIRCam-x86_64.AppImage` (Ubuntu 22.04 / old glibc)
- **Flatpak** — `SuperIRCam.flatpak` (KDE Platform 6.10, OpenCV in the sandbox)

Open the latest green **Portable packages** run and download the artifacts:

https://github.com/stulluk/super-ircam/actions

Verified on Debian sid/forky and Ubuntu 22.04. Your user should be in the
`video` group. Plug the thermal module in first; after NUC/FFC (about 3
seconds of `0x8000` frames) the image and overlays appear.

### AppImage

```bash
chmod +x SuperIRCam-x86_64.AppImage
./SuperIRCam-x86_64.AppImage
```

Ubuntu 22.04 needs `libfuse2` for double-click / FUSE (`sudo apt install
libfuse2`). Without it you can still run:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./SuperIRCam-x86_64.AppImage
```

### Flatpak

```bash
flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user SuperIRCam.flatpak
flatpak run com.github.stulluk.SuperIRCam
```

The first install pulls `org.kde.Platform//6.10` from Flathub. The
manifest grants `--device=all` so raw V4L2 can open `/dev/video*` (the
GNOME/KDE camera portal is not enough for this UVC radiometry node).

### Build locally with Docker

Host `-dev` packages are not required.

```bash
./scripts/docker_build_appimage.sh
./scripts/docker_build_flatpak.sh   # needs privileged Docker (bwrap)
```

Outputs land in `dist/`.

Thermal-Camera-Redux is a separate project (CLI `redux`, Docker `.deb`).
The public Super IRCam tree does not vendor it:

https://github.com/stulluk/Thermal-Camera-Redux

## Optional CLI flags

```bash
./SuperIRCam-x86_64.AppImage --screenshot "$HOME/Pictures/window.png" --quit-after-shot
flatpak run com.github.stulluk.SuperIRCam --record-seconds 2
```

## Two-column control panel

The right-hand controls use two columns so a typical window height does not
need a scrollbar. If you prefer the older single-column scrolling sidebar,
revert the commit that introduced the two-column layout (it is a standalone
change on `main`).
