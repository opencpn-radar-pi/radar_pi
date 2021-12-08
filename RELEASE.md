How to release the plugin for "unstable" or "stable" release
============================================================

To release a new version of the plugin using the _managed plugins_ is a two step
process:

1. Publish the "download files" on Cloudsmith.
2. Create a pull request for the OpenCPN plugin project.

There is actually a third step involved, where the OpenCPN developers merge the pull request,
but we don't need to document their work here.

For step 2 you need a Linux or macOS+homebrew installation, with the 'jq' and 'curl' tools installed.

## Publish the "download files" on Cloudsmith

If you `git push` a version that is _not_ tagged, it ends up in the _unstable_ repository at
https://cloudsmith.io/~opencpn-radar-pi/repos/opencpn-radar-pi-unstable/packages/
which is fine; you can test that the code builds. Do not worry about cleaning this up; cloudsmith will do this
automatically.

However, at some point, the time comes to release a Beta or Production release.

We release our packages with the build number and short git commit hash, so you can repeat the procedure below
until you are satisfied and all binaries are built correctly. You should remove failed builds (where not all
products are built) from Cloudsmith. We keep successful old builds (e.g. older versions) in the Beta and Prod
repositories so that people that have not upgraded their catalog can still download old releases.

Follow these steps:

1. Check that you are using the _master_ branch.
    ```
    git checkout master
    ```

2. Edit the following lines in Plugin.cmake to reflect the new status:
    ```
#
# -------  Plugin setup --------
#
set(PKG_NAME radar_pi)
set(PKG_VERSION 5.3.0)
set(PKG_PRERELEASE "beta")  # Empty, or a tag like 'beta'
    ```

3. For Beta releases: Add the file to the git staging area, add a commented tag that includes Beta or beta:

    ```
    git add Plugin.cmake
    git commit -m"v5.0.4-beta1 release"
    git tag -a --force -m"v5.0.4-beta1 release" v5.0.4-beta1
    git push --atomic --force origin master v5.0.4-beta1
    ```
  
   Note that the `--force` tag is only needed if this is a 2nd try where the tag already exists locally
   and/or on the server.
   We use this complicated version of git push instead of `git push; git push --tags` so that only
   one CI build is kicked off. If you _want_ to test a build to the `Unstable` repository first,
   go ahead and use `git push` (and only if that gives what you want, run `git push --tags`.)

4. For Production releases: Add the file, commit, add a commented tag and push both code and tags in 1 step:
    ```
    git add CMakeLists.txt
    git commit -m"v5.0.4 release"
    git tag -a --force -m"v5.0.4 release" v5.0.4
    git push --atomic --force origin master v5.0.4
    ```

If you wait a while you will see builds turn up in Cloudsmith, built by Appveyor, Drone and Cloud CI. 
See below for the URLs. You can also follow the links in Github on the commit (there is a small progress
dot or a checkmark correct/bad nowadays).

The process of adding and pushing the tag will put the releases in the "stable" repository on Cloudsmith.

That concludes the building actions.

## Where can I find the builds

You can check the build progress at the following sites.

[Circle CI pipelines](https://app.circleci.com/github/opencpn-radar-pi/radar_pi/pipelines) for Linux x86 and macOS builds
[Drone](https://cloud.drone.io/opencpn-radar-pi/radar_pi) for Raspberry Pi (armhf) builds
[Appveyor](https://ci.appveyor.com/project/keesverruijt/radar-pi) for Microsoft Windows

### New cloudsmith packages (>= 5.1.5)

[CloudSmith Prod Packages](https://cloudsmith.io/~opencpn-radar-pi/repos/opencpn-radar-pi-prod/packages/)
[CloudSmith Beta Packages](https://cloudsmith.io/~opencpn-radar-pi/repos/opencpn-radar-pi-beta/packages/)

### Old cloudsmith packages (<= 5.1.4)

These were used for old releases, once we have 5.2.0 out in both production and beta they can be removed:

[CloudSmith Stable Packages](https://cloudsmith.io/~kees-verruijt/repos/ocpn-plugins-stable/packages/)
[CloudSmith Unstable Packages](https://cloudsmith.io/~kees-verruijt/repos/ocpn-plugins-unstable/packages/)

## Create a pull request for `opencpn-plugins`

The [opencpn-radar-pi](https://github.com/opencpn-radar-pi) organization already contains a fork 
of [OpenCPN/plugins](https://github.com/OpenCPN/plugins) at
[opencpn-radar-pi](https://github.com/opencpn-radar-pi/plugins).

1. Clone the _opencpn-radar-pi_ repo, and set upstream, if you have not already done so:
   (See [Configuring a remote for a fork](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/configuring-a-remote-for-a-fork))
    ```
    git clone git@github.com:opencpn-radar-pi/plugins.git` 
	or
	git clone https://github.com/opencpn-radar-pi/plugins
    cd plugins
    git remote add upstream https://github.com/OpenCPN/plugins.git
    ```

2. Sync the fork with the latest info from upstream 
   (See [Syncing a fork](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/syncing-a-fork))
    ```
    git fetch upstream
    git checkout master
    git pull
    git merge -s recursive -X theirs upstream/master
    ```
   or for unstable:
    ```
    git fetch upstream
    git checkout Beta
    git pull
    git merge -s recursive -X theirs upstream/Beta
    ```

    The `fetch upstream` gets the work done upstream, but doesn't merge yet. The `checkout <branch>` moves
    your version to that branch, but just the version as you had it before. Since others in the team may have
    made a release, you need to merge those first with `pull`. Now you are ready to update to the upstream
    versio. Since that may have changed dramatically, we merge this in a way that always takes 'their' files
    not 'ours'. This should give no conflicts.

	Alternatively the syncing of the can be done on the main git page: https://github.com/opencpn-radar-pi/plugins by clicking "Fetch upstream".
	This Fetch upstream should be done for the Beta branch when working on beta.

3. Copy the XML files from CloudSmith to your local plugins repo:
    ```
    ./cloudsmith-sync.sh radar_pi opencpn-radar-pi opencpn-radar-pi-prod 5.2.0.eecba41
    ```
   Or for unstable/Beta:
    ```
    ./cloudsmith-sync.sh radar_pi opencpn-radar-pi opencpn-radar-pi-beta 5.2.0-beta4.0.eecba41

    ```

   Unlike earlier versions of the sync script you must determine yourself what version+commit
   to download. As this is a search key it should identify the release, does not need the full version+commit. Radar-v5.3.0-beta worked fine.

   The output should be verbose with each XML url mentioned as well as the XML for that file.
   If you don't, run it using `sh -x`, copy the shown curl command and run this yourself to see what you are doing wrong.


4. Check that there are new radar_pi files in the `metadata` subdirectory. Remove any old files, for instance:

    ls metadata/*radar*
    # Yes, see 5.2.0-beta3 (old) and 5.2.0-beta4 (new) files
    rm metadata/*radar*-5.2.0-beta3 

   Check that there are no missing platforms, certainly for a stable release.
   The new files will be added by `git add .` in step 5.

6. Create a Pull Request:
    ```
    git add .
    git commit -m"Radar plugin version X.YZ"
    git push
    ```

    DO NOT RUN MAKE WHICH UPDATES OCPN-PLUGINS.XML ! This will be done later by bdbcat
    when he merges the request.

7. Go to github.com and create a Pull Request for `OpenCPN/plugins`

   For stable/master: https://github.com/OpenCPN/plugins/compare/master...opencpn-radar-pi:master?expand=1
   For unstable/Beta: https://github.com/OpenCPN/plugins/compare/Beta...opencpn-radar-pi:Beta?expand=1

