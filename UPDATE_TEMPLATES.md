updates-templates README
========================

The script update-templates can be used to update a plugin with
newer versions of the shipdriver templates. The basic workflow
is to

  - Make sure the plugin repo is clean (commit or stash changes)
  - Download the script
  - Run it
  - Inspect the results, handle possible conflicts and commit the
    changes.


Downloading
-----------
Linux:

    $ cd some_plugin
    $ repo=https://raw.githubusercontent.com/leamas/shipdriver_pi/update-164
    $ curl $repo/update-templates --output update-templates

It is also possible to use wget instead of curl, like
`wget $repo/update-templates`.


Downloading - Windows
---------------------

The script is written in bash, so git-bash is required. Using git-bash, the
script can be run as in Linux, see above.

Using the command CLI goes like:

    > cd some_plugin
    > set repo=https://raw.githubusercontent.com/leamas/shipdriver_pi/update-164
    > curl %repo%/update-templates --output update-templates

Running
-------

The script is run from the plugin top directory using
`bash ./update-templates`. In windows, assuming standard installation paths:

    > "C:\Program Files\Git\bin\bash.exe" update-templates

There are two options:
  - *-t treeish*
       The source treeish in the shipdriver repository. Defaults to
       *shipdriver/master*
  - *-b branch*
       The destination branch where changes are merged. Defaults to
       *main* if it exists, otherwise *master*



Inspecting results and committing
---------------------------------

The basic check is `git status`. This will display a list of modified or
added files. `git diff --staged <filename>` shows how a specific file is
modified.

If the template files have local changes there might be conflicts. These
can be resolved manually by editing the conflicting file.

All changes can be reverted using `git checkout HEAD <file>` -- doing this
on a conflicted file resolves the conflict.

Another option is to apply all changes unconditionally, basically dropping
local changes using `git checkout shipdriver/master <file>` -- this also
resolves a possible conflict.

Note that changes might be required in other files like CMakeLists.txt.

When all looks good changes can be committed using something like
`git commit -m "Update shipdriver templates."`



Pin files which should not be updated
-------------------------------------

The script supports a file named *update-ignored*. This is a list of files,
one per line, which should not be updated in any case.
