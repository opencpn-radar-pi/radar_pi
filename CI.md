Shipdriver Continous Integration README
---------------------------------------

This document is designed to be generic and terse. More complete 
documentation is available at the Opencpn Developer Manual [[6]](#fn6).

This software uses a continous integration (CI) setup which builds and
deploys artifacts which can be used by the new plugin installer. The
setup is based on storing the sources on github.

In a first step, the software is built. This step uses builders from
appveyor[[1]](#fn1), circleci[[2]](#fn2) and travis[[3]](#fn3) to make
around 10  builds for different platforms. Each build  produces an xml
metadata file and a tarball.

In next step the build artifacts are uploaded to a cloudsmith [[4]](#fn4)
deployment server.

In a last step, the metadata XML file is pushed to a remote git clone
of the opencpn plugins project.

All steps could be configured after cloning the shipdriver\_pi sources.

Builders
--------

The appveyor builder can be activated after registering a free account
on the appveyor website. After creating the account, start following the 
project. From this point a new build is initiated as soon as a commit is
pushed to github. Configuration lives in `appveyor.yml`.

The circleci and travis builders works the same way: register an account
and start following the project. The builds are triggered after a commit
is pushed to github. Configuration lives in `.circleci/config.yml` and
`.travis.yml`.

Circleci requires user to manually request a MacOS builder before it can
be used[[11]](#fn11). This process has been working flawlessly.


Cloudsmith
----------
To get the built artifacts pushed to cloudsmith after build (assuming all
builders are up & running):

  - Register a free account on cloudsmith.
  - Create three open-source repositories [[10]](#fn10) called for example
    *opencpn-plugins-stable*, *opencpn-plugins-unstable* and
    *opencpn-plugins-beta*.
  - Set up the following environment variables in the travis[[7]](#fn7),
    circleci[[8]](#fn8) and appveyour[[9]](#fn9) builders:

     - **CLOUDSMITH_API_KEY**: The value of the API key for the cloudsmith
       account (available in the cloudsmith ui).
     - **CLOUDSMITH_UNSTABLE_REPO**: The name of the repo for unstable
       (untagged) commits. My is *alec-leamas/opencpn-plugins-unstable*
     - **CLOUDSMITH_BETA_REPO**: The name of the repo for tagged builds
       with a tag name matching "beta".
     - **CLOUDSMITH_STABLE_REPO**: The name of the repo for stable (tagged)
       commits which doesn't go inte the beta repository.

After these steps, the artifacts built should start to show up in cloudsmith
when built. The builder logs usually reveals possible errors.

The setup is complete when cloudsmith repos contains tarball(s) and metadata 
file(s). It is essential to check that the url in the metadata file matches
the actual url to the tarball.

Git plugins repository sync
---------------------------

The git sync basically automates pushing built metadata files to a git
repository using a flow like:

    $ git clone ${PLUGINS_REPO}
    $ cp plugin_pi.xml plugins/metadata
    $ cd plugins
    $ git commit -am "Updating plugins_pi.xml"
    $ git push origin master:master

This makes it easy to make a pull request against the plugins project
to publish new plugin versions. The details are described in README-git.md



Using the generated artifacts
-----------------------------

The generated tarball could be installed using the "Import Plugin" button in
the plugin manager window (assuming version 5.2.0+)

To make the generated plugins visible in the list of available plugins a new
ocpn-plugins.xml file needs to be generated. This is out of scope for this
document, see the opencpn plugins project [[5]](#fn5).

<div id="fn1"/> [1] https://www.appveyor.com/ <br>
<div id="fn2"/> [2] https://circleci.com/ <br>
<div id="fn3"/> [3] https://travis-ci.com/ <br>
<div id="fn4"/> [4] https://cloudsmith.io/~david-register/repos/ <br>
<div id="fn5"/> [5] https://github.com/opencpn/plugins <br>
<div id="fn6"/> [6] https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:developer_manual <br>

<div id="fn7"/> [7] https://docs.travis-ci.com/user/environment-variables/#defining-variables-in-repository-settings <br>
<div id="fn8"/> [8] https://circleci.com/docs/2.0/env-vars/#setting-an-environment-variable-in-a-project <br>
<div id="fn9"/> [9] https://www.appveyor.com/docs/build-configuration/#environment-variables <br>
<div id="fn10"/> [10] https://help.cloudsmith.io/docs/create-a-repository <br>
<div id="fn11"/> [11] https://circleci.com/open-source/ <br>

