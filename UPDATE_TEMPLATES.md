updates-templates README
========================

The script update-templates can be used to update a plugin with
newer versions of the shipdriver templates. The basic workflow
is to
  - If keys exist in the ./ci folder copy them to ./build-deps.
  - Bootstrap process by downloading the updates-templates script
    and add it to repo if it does not exist.
  - Make sure the plugin repo is clean (commit or stash changes)
  - Pin files which should not be updated.
  - Run script
  - Inspect the results.
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


Pin files which should not be updated
-------------------------------------

If there are files which are known to have local modifications, list these
files (one per line) in a file named *update-ignored*.  This file is not
present by default, and needs to be created and committed if used.


Running
-------

The script is run from the plugin top directory using
`./update-templates`. In windows CMD, assuming standard installation paths:

    > "C:\Program Files\Git\bin\bash.exe" update-templates

Usage summary:

    update-templates [-T] [treeish]
    update-templates  -h
    update-templates  -l
    
**treeish** defaults to _shipdriver/master_ i. e., templates are updated
from shipdriver's development branch. It could be set to a branch
like _shipdriver/v3.0_ or a tag like _sd3.0.0_ to retrieve data from
corresponding git trees.

**-l** lists available tags which can be used as _treeish_

**-T** runs in test mode, lots of output, requires an existing shipdriver 
remote and does not self-update.

*update-templates -h* prints the complete help message.

Script unconditionally updates known files and commits them directly.

Examples:
 
    update-templates shipdriver/v3.0    -- get updates from v3.0 branch
    update-templates sd3.0.0            -- get updates from sd3.0.0 tag
    update-templates -l                 -- list all available tags

Checking modifications in CMakeLists.txt and flatpak manifest
-------------------------------------------------------------

As part of the 3.0.0 transition CMakeLists.txt is split into one plugin-specific 
file Plugin.cmake and a generic CMakeLists.txt.  Later updates
are only supposed to affect CMakeLists.txt while Plugin.cmake, the
plugin-specific parts is kept as-is.

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
