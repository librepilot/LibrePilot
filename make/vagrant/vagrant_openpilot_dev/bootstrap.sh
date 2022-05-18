#!/usr/bin/env bash

sudo apt-get update
sudo apt-get --yes --force-yes upgrade

# get ourselves some basic toys
sudo apt-get --yes --force-yes install git-core build-essential openssl libssl-dev 

# install some tools for the vagrant user only by executing
# a script AS the vagrant user
cp /vagrant/install-tools.sh /home/vagrant
chown vagrant /home/vagrant/install-tools.sh
chgrp vagrant /home/vagrant/install-tools.sh
chmod ug+x /home/vagrant/install-tools.sh
su -c "/home/vagrant/install-tools.sh" vagrant

