updates-templates README
========================

The script update-templates can be used to update a plugin with
newer versions of the shipdriver templates. The basic workflow
is to

  - Make sure the plugin repo is clean (commit or stash changes)
  - Bootstrap process by downloading script and add it to repo.
  - Run script
  - Inspect the results, handle possible conflicts and commit the
    changes.


Bootstrapping
-------------
Only required if the update script is yet not part of the repo. Once
installed, the script is self-updating.

Linux:

    $ cd some_plugin
    $ repo=https://raw.githubusercontent.com/Rasbats/shipdriver_pi/master
    $ curl $repo/update-templates --output update-templates
    $ chmod 755 update-templates
    $ git add update-templates
    $ git commit -m "Add update-templates script"

It is also possible to use wget instead of curl, like
`wget $repo/update-templates`.


Bootstrap - Windows
-------------------

As in linux, bootstrapping is only required if the script is yet not
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
`./update-templates`. In windows, assuming standard installation paths:

    > "C:\Program Files\Git\bin\bash.exe" update-templates

Usage summary:

    update-templates [-t] [treeish]

The *-t* option adds a *-X theirs* to the git merge performed. It will 
resolve all conflicts by using the upstream shipdriver stuff. By default,
the conflicts will be unresolved in the results.

The *treeish* option can be used to merge changes from another shipdriver
state than the default shipdriver/master i. e., a tag or a commit from
the shipdriver repo.

*update-templates -h* prints the complete help message.


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
