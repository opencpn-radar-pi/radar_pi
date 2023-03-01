#  Create a wxWidgets 3.2 backport to bullseye in /usr/local/pkg
#

# single backport patch to bookworm source
function create_patch() {
cat << EOF >patch1.patch
diff --git a/debian/changelog b/debian/changelog
index 9835d62..a58fc47 100644
--- a/debian/changelog
+++ b/debian/changelog
@@ -1,3 +1,10 @@
+wxwidgets3.2 (${vers}) bullseye-backports; urgency=medium
+
+  * local package
+  * Rebuild for bullseye-backports.
+
+ -- Alec Leamas <leamas.alec@gmail.com>  Sat, 19 Nov 2022 13:07:44 +0100
+
 wxwidgets3.2 (3.2.1+dfsg-1) unstable; urgency=medium

   * Update to new upstream release 3.2.1
--
2.30.2
EOF
}


# Remove existing wx30 packages
function remove_wx30() {
  sudo apt remove \
      libwxsvg3 \
      wx3.0-i18n \
      wx-common \
      libwxgtk3.0-gtk3-0v5 \
      libwxbase3.0-0v5 \
      libwxsvg3 wx3.0-i18n \
      wx-common \
      libwxgtk3.0-gtk3-0v5 \
      libwxbase3.0-0v5 wx3.0-headers
}

# Install generated packages
function install_wx32() {
  test -d /usr/local/pkg || mkdir /usr/local/pkg
  sudo chmod a+w /usr/local/pkg
  repo="https://dl.cloudsmith.io/public/alec-leamas/wxwidgets"
  head="deb/debian/pool/bullseye/main"
  vers="3.2.1+dfsg-1~bpo11+1"
  pushd /usr/local/pkg
  wget $repo/$head/w/wx/wx-common_${vers}/wx-common_${vers}_amd64.deb
  wget $repo/$head/w/wx/wx3.2-i18n_${vers}/wx3.2-i18n_${vers}_all.deb
  wget $repo/$head/w/wx/wx3.2-headers_${vers}/wx3.2-headers_${vers}_all.deb
  wget $repo/$head/l/li/libwxgtk-webview3.2-dev_${vers}/libwxgtk-webview3.2-dev_${vers}_amd64.deb
  wget $repo/$head/l/li/libwxgtk-webview3.2-0_${vers}/libwxgtk-webview3.2-0_${vers}_amd64.deb
  wget $repo/$head/l/li/libwxgtk-media3.2-dev_${vers}/libwxgtk-media3.2-dev_${vers}_amd64.deb
  wget $repo/$head/l/li/libwxgtk3.2-dev_${vers}/libwxgtk3.2-dev_${vers}_amd64.deb
  wget $repo/$head/l/li/libwxgtk3.2-0_${vers}/libwxgtk3.2-0_${vers}_amd64.deb
  wget $repo/$head/l/li/libwxbase3.2-0_${vers}/libwxbase3.2-0_${vers}_amd64.deb
  sudo apt install -y $(ls /usr/local/pkg/*deb)
  popd
}

if [ -d /usr/local/pkg ]; then
  echo "wxWidgets32 packages already in place"
else
  set -x
  echo "Cloning wxWidgets sources"
  mkdir wxWidgets; cd wxWidgets
  create_patch;
  git clone https://gitlab.com/leamas/wxwidgets3.2.git
  cd wxwidgets3.2
  git fetch origin master:master
  git fetch origin pristine-tar:pristine-tar
  
  echo "Creating the .orig tarball"
  git checkout pristine-tar
  pristine-tar checkout $(pristine-tar list | tail -1)
  mv *xz ..
  rm *
  
  echo "Patch and building package"
  git checkout master
  patch -p1 < ../patch1.patch
  git checkout .; git clean -fxd
  mk-build-deps --root-cmd=sudo -i -r
  rm *changes *buildinfo
  debuild -us -uc
  
  echo "Installing pkg in /usr/local/pkg"
  test -d /usr/local/pkg || sudo mkdir /usr/local/pkg
  sudo cp ../*deb /usr/local/pkg
  sudo rm /usr/local/pkg/*dbgsym* 
  cd ../...
fi

