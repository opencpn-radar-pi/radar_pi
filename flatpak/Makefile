PLUGIN = org.opencpn.OpenCPN.Plugin.shipdriver

ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))


all: build

build:
	flatpak-builder --repo=repo --force-clean \
	    app $(ROOT_DIR)/$(PLUGIN).yaml

install:
	flatpak install --user --reinstall -y $(CURDIR)/repo $(PLUGIN)

uninstall:
	flatpak uninstall --user $(PLUGIN)


