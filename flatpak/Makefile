PLUGIN = org.opencpn.OpenCPN.Plugin.radar

all: build

build:
	flatpak-builder --force-clean app $(PLUGIN).yaml
	flatpak build-export repo app master

install:
	flatpak install --user --reinstall -y $(CURDIR)/repo $(PLUGIN)

uninstall:
	flatpak uninstall --user $(PLUGIN)
