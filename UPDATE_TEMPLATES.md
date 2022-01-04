updates-templates README
========================

The script update-templates can be used to update a plugin with
newer versions of the shipdriver templates. The basic workflow
is to
  - If keys exist in the ./ci folder copy them to ./build-deps.
  - Bootstrap process by downloading the updates-templates script
    and add it to repo if it does not exist.
  - Make sure the plugin repo is clean (commit or stash changes)
  - Run script
  - Handle updates to CMakeLists.txt/Plugin.cmake and the
    flatpak yaml manifest.
  - Upstream local changes to shipdriver templates

Bootstrapping
-------------
Only required if the update script is not yet part of the repo. Once
installed, the script is self-updating.

Linux:

    $ cd some_plugin
    $ repo=https://raw.githubusercontent.com/Rasbats/shipdriver_pi/master
    $ curl $repo/update-templates --output update-templates
    $ chmod 755 update-templates
    $ git add update-templates
    $ git commit -m "Add update-templates script"

It is also possible to use wget instead of curl, like
`wget $repo/update-templates`. Windows works the same way when using git-bash,
except that the `chmod` command does not make sense here and hence is omitted.


Bootstrap - Windows (cmd.com)
-----------------------------

As in linux, bootstrapping is only required if the script is not yet
available in the plugin repo. Once installed, it's self-updating.

The script is written in bash, so git-bash is required. Using git-bash, the
script can be downloaded and installed as in linux, see above.

Using the Windows command CLI goes like:

    > cd some_plugin
    > set repo=https://raw.githubusercontent.com/Rasbats/shipdriver_pi/master
    > curl %repo%/update-templates --output update-templates
    > git add update-templates
    > git commit -m "Add update-templates script"


Running
-------

The script is run from the plugin top directory using
`./update-templates`. In windows CMD, assuming standard installation paths:

    > "C:\Program Files\Git\bin\bash.exe" update-templates

Usage summary :

    update-templates [-T]  <treeish>
    update-templates <-h|-l>

Parameters:

**treeish**: A shipdriver tag or branch.  Recommended usage is using the
latest stable (non-beta) tag.

Options:

**-l** lists available tags which can be used as _treeish_

**-T** runs in test mode, lots of output, requires an existing shipdriver 
remote and does not self-update.

**-h** prints the complete help message.

Examples:

    update-templates -l                   -- List available tags
    update-templates sd3.0.1              -- Update from sd3.0.1 tag
    update-templates shipdriver/v3.0      -- Update from v3.0 release branch
    update-templates shipdriver/master    -- Update from development branch

Checking modifications in CMakeLists.txt and flatpak manifest
-------------------------------------------------------------

As part of the 3.0.0 transition CMakeLists.txt is split into one plugin-specific 
file Plugin.cmake and a generic CMakeLists.txt. If the file _Plugin.cmake_
exists script will thus update _CMakeLists.txt_, otherwise not.

The "flatpak manifest" is the yaml file configuring the flatpak build,
named like flatpak/org.opencpn.OpenCPN.Plugin.\*.yaml.  This might need
to be updated. If there have been changes to the shipdriver manifest 
since the last release these are added as comments at the end of the
manifest. Review the file, consider applying corresponding changes to
the manifest and eventually remove the comment.


Upstreaming local changes to shipdriver
---------------------------------------
If there is a need to modify any of the files updated by update-templates,
please file bugs against the shipdriver repo so the next update runs smoother.
