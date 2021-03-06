# Developers

<!-- update the table of contents with: doctoc --github DEVELOPERS.md -->
<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Build for Linux and Windows](#build-for-linux-and-windows)
- [Build for macOS](#build-for-macos)
- [Use Visual Studio Code for development and debugging](#use-visual-studio-code-for-development-and-debugging)
- [Take screenshots for the website](#take-screenshots-for-the-website)
- [Update third party dependencies](#update-third-party-dependencies)
  - [boost](#boost)
  - [tvision](#tvision)
  - [mpdecimal](#mpdecimal)
  - [nameof](#nameof)
- [Make a release build](#make-a-release-build)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

___

## Build for Linux and Windows
Linux and Windows builds must be produced on a Linux build machine.

1. Install Docker and give your user account permission to run `docker`.

    **Ubuntu Linux** (on AWS, use an official AMI)

    ```
    sudo apt-get update -y && \
    sudo apt-get upgrade -y && \
    sudo apt-get install -y git && \
    sudo curl -fsSL https://get.docker.com -o get-docker.sh && \
    sudo sh get-docker.sh && \
    sudo usermod -aG docker ubuntu && \
    sudo reboot
    ```

    **Arch Linux** (on AWS, use a community AMI from [Uplink Laboratories](https://www.uplinklabs.net/projects/arch-linux-on-ec2/))

    ```
    sudo pacman -Syu --noconfirm && \
    sudo pacman -S --noconfirm docker git zip && \
    sudo usermod -aG docker arch && \
    sudo systemctl enable docker && \
    sudo reboot
    ```

1. Clone the git repository.

    ```
    git clone https://github.com/electroly/tmbasic.git
    cd tmbasic
    ```

1. Start the build environment and compile TMBASIC.

    ```
    cd build
    ./dev.sh
    make
    ```

    This will create a development build for Linux suitable for debugging.
    This build of TMBASIC will be unable to produce executables for other platforms because it does not contain the necessary builds of the interpreter. See the "Make a release build" instructions below to produce a TMBASIC build that can produce executables for all platforms.

    Use one of the `linux-*.sh` or `win-*.sh` scripts instead of `dev.sh` to produce a non-debug build for Linux or Windows on a particular architecture.

    **Troubleshooting**: If you get an error from `pacman` when starting the Docker environment for Windows, try an Arch Linux host machine.
    Fatal incompatibilities have occurred when running the Arch-based container on an Ubuntu host.

1. Type `exit` to leave the build environment.

## Build for macOS
1. Install Xcode (version 10 or higher).

1. Start the build environment and compile:

    ```
    cd build
    ./mac-x64.sh
    make
    ```

1. Type `exit` to leave the build environment.

## Use Visual Studio Code for development and debugging
1. Install the `ms-vscode.cpptools` and `ms-vscode-remote.remote-containers` extensions in Visual Studio Code.
1. Start the container using the instructions above.
1. In Visual Studio Code, click "View" > "Command Palette..." and run the "Remote-Containers: Attach to Running Container..." command.
1. Choose the `/tmbasic-dev` container.
1. Choose the `/code` directory.

## Take screenshots for the website
SVG screenshots would have been nice, but they get garbled in some browsers (Chrome on Android). Instead, we will just take regular PNG screenshots.

- Windows 10 at 175% scaling
- PuTTY
- Terminal size: 80x24
- Window > Appearance
    - Cursor appearance: Underline
    - Font: Consolas 14pt
    - Font quality: Non-Antialiased
- Window > Colours > ANSI Cyan: 58, 150, 221

Crop to the console area including the one pixel black outline. Post-process with:

```
pngcrush -brute -reduce -ow screenshot.png
```

## Update third party dependencies

Download links for libraries are located in the following files:

- `build/mac-x64.sh`
- `build/files/Dockerfile.build-dev`
- `build/files/Dockerfile.build-linux`
- `build/files/Dockerfile.build-win`

### boost
1. https://www.boost.org/
    - Click "Download" under "Current Release".
    - Download the tarball.
    - Copy the link.
1. Edit `ext/README.md` and replace the boost link.
1. Upload to S3: `aws s3 cp boost_X_X_X.tar.gz s3://tmbasic/boost/ --acl public-read` (use the downloaded filename)
1. Edit `ext/README.md` and replace the mirror link.
1. Update the URL in the files listed above.
1. Commit as "Update boost to version ____"

### tvision
1. https://github.com/magiblot/tvision
    - Click commit hash
    - Click "Browse files"
    - Click "Code"
    - Right-click "Download ZIP"
    - Click "Copy link"
    - Click "Download ZIP" to download it
1. Edit `ext/README.md` and replace the tvision link.
1. Upload to S3: `aws s3 cp tvision-____.zip s3://tmbasic/tvision/ --acl public-read` (use the downloaded filename)
1. Edit `ext/README.md` and replace the mirror link.
1. Update the URL in the files listed above.
1. Commit as "Update tvision to commit ____"

### mpdecimal
1. https://www.bytereef.org/mpdecimal/download.html
    - Download the stable version tarball.
    - Copy the link.
1. Edit `ext/README.md` and replace the mpdecimal link.
1. Upload to S3: `aws s3 cp mpdecimal-X.X.X.tar.gz s3://tmbasic/mpdecimal/ --acl public-read` (use the downloaded filename)
1. Edit `ext/README.md` and replace the mirror link.
1. Update the URL in the files listed above.
1. Commit as "Update mpdecimal to version ____"

### nameof
1. https://github.com/Neargye/nameof/releases/
1. Copy the `nameof.hpp` link. Use `wget` to download, overwriting `ext/nameof/nameof.hpp`.
1. Update link in `ext/README.md`.
1. Commit as "Update nameof to version ____"

## Make a release build
1. Start the following three build machines. Prepare them for building using the instructions at the beginning of this document.

    - Ubuntu Linux on ARM64 (AWS `c6g.large`)
    - Arch Linux on x64 (AWS `c5a.large`)
    - macOS 10.13 on x64 (MacinCloud PAYG)

1. Clear any existing dependencies on the three build machines so that we perform a fresh build using the latest versions.

    **Linux**

    ```
    docker system prune -a
    ```

    **Mac**

    ```
    rm -rf mac
    ```

1. Perform the rest of these instructions on the Mac. Make sure that the two Linux instances are accessible via `ssh` with public key authentication. Copy the private keys (`.pem` files) onto the Mac. Set the permissions on the key files:

    ```
    chmod 600 filename.pem
    ```

1. Configure your bash session with the `ssh` connection details for the Linux ARM64 and x64 build machines using the following commands.

    ```
    export ARM_KEY=/path/to/arm64-ssh-key.pem
    export ARM_USER=ubuntu
    export ARM_HOST=arm64-hostname-or-ip
    export X64_KEY=/path/to/x64-ssh-key.pem
    export X64_USER=arch
    export X64_HOST=x64-hostname-or-ip
    ```

1. Run all tests on all platforms by running the following command.

    ```
    pushd build/test && ./test.sh && popd
    ```

1. Produce a distribution-ready production build for each platform by running the following command.

    ```
    pushd build/publish && ./publish.sh && popd
    ```

    Output files will appear in the `dist` directory.
