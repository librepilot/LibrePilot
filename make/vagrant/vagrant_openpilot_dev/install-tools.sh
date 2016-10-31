#!/usr/bin/env bash

echo "
Setup a build environment
"

ANDROID_ENV=false
ANDROID_STUDIO_VERSION=1.1.0
ANDROID_STUDIO_BUILD=135.1740770
ANDROID_STUDIO_FILE=android-studio-ide-$ANDROID_STUDIO_BUILD-linux.zip

ANDROID_SDK_VERSION=r24.1.2
ANDROID_SDK_FILE=android-sdk_$ANDROID_SDK_VERSION-linux.tgz
ANDROID_SDK_URL=http://dl.google.com/android/${ANDROID_SDK_FILE}
ANDROID_API_LEVELS=android-20,android-21,android-22
ANDROID_BUILD_TOOLS_VERSION=21.1.2

# Setup a build environment
sudo add-apt-repository --yes ppa:librepilot/tools
sudo apt-get --yes --force-yes update
sudo apt-get --yes --force-yes install build-essential ccache debhelper git-core git-doc flex graphviz bison libudev-dev libusb-1.0-0-dev libsdl1.2-dev python libopenscenegraph-dev qt56-meta-minimal qt56svg qt56script qt56serialport qt56multimedia qt56translations qt56tools qt56quickcontrols libosgearth-dev openscenegraph-plugin-osgearth
sudo apt-get --yes --force-yes install libc6-i386


# Do Android stuff
if [ "$ANDROID_ENV" = "true" ]; then

	# install java7
	sudo echo "deb http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | sudo tee /etc/apt/sources.list.d/webupd8team-java.list
	sudo echo "deb-src http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | sudo tee -a /etc/apt/sources.list.d/webupd8team-java.list
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys EEA14886
	sudo apt-get update
	# accept the license agreement
	echo oracle-java7-installer shared/accepted-oracle-license-v1-1 select true | sudo /usr/bin/debconf-set-selections

	sudo apt-get --yes --force-yes install oracle-java7-installer

	# make a place to install development tools
	mkdir -p ~/workspace/tools
	cd ~/workspace/tools

	# download and unpack android-studio
	wget https://dl.google.com/dl/android/studio/ide-zips/$ANDROID_STUDIO_VERSION/$ANDROID_STUDIO_FILE
	unzip $ANDROID_STUDIO_FILE
	rm $ANDROID_STUDIO_FILE

	# create a launcher for android-studio
	mkdir /home/vagrant/Desktop/
	echo "[Desktop Entry]
	Version=1.0
	Type=Application
	Name=Android-Studio
	Comment=
	Exec=/home/vagrant/workspace/tools/android-studio/bin/studio.sh
	Icon=/home/vagrant/workspace/tools/android-studio/bin/idea.png
	Path=/home/vagrant/workspace/tools/android-studio
	Terminal=false
	StartupNotify=false
	GenericName=" >> "/home/vagrant/Desktop/Android-Studio.desktop"

	chmod u+x /home/vagrant/Desktop/Android-Studio.desktop

	# download android sdk
	wget http://dl.google.com/android/$ANDROID_SDK_FILE
	tar -zxf $ANDROID_SDK_FILE
	rm $ANDROID_SDK_FILE

	# install android sdk extras to get google libs
	ANDROID=/home/vagrant/workspace/tools/android-sdk-linux/tools/android
	echo y | $ANDROID update sdk --no-ui --filter extra-android-support,extra-android-m2repository,extra-google-m2repository,tools,platform-tools,${ANDROID_API_LEVELS},build-tools-${ANDROID_BUILD_TOOLS_VERSION},build-tools-20.0.0

fi

# Checkout code
mkdir -p ~/workspace/
cd ~/workspace/
git clone https://bitbucket.org/librepilot/librepilot.git
cd librepilot
git checkout next

# Dev Tools Installation
make build_sdk_install
