How to release the plugin for public consumption
================================================

1. Edit the following lines in CMakeLists.txt to reflect the new status:
```
    #SET(CMAKE_BUILD_TYPE Debug)
    SET(VERSION_MAJOR "5")
    SET(VERSION_MINOR "0")
    SET(VERSION_PATCH "4-beta1")
    SET(VERSION_DATE "2019-09-08")
```
2. Add the file to the git staging area, commit this then tag the commit with the same version number:
```
    git add CMakeLists.txt
    git commit -m"v5.0.4-beta1"
    git tag "v5.0.4-beta1"
```

3. Push the version, including tags, to github.
```
    git push --tags
```

That's it! The process of adding the tag will make Appveyor and Travis build a release version and create a release in Github for
the project automatically. Wait for the first artifact (usually Windows from Appveyor) to turn up as a release, then modify the full description of the release to include the release notes / bugs solved etc.


_Wrong commit tagged?_

As long as you haven't done `git push --tags` yet you can move the tag to the correct commit by using `git tag -f <tagname> <commitid>` or if the desired commit is the new HEAD (last commit) then `git tag -f <tagname>`.

_Checking if the code builds on macOS + Linux + Windows_

Just push a version, just don't tag it -- Appveyor and Travis will still build it, but you need to go to their website to find the result.
* [https://ci.appveyor.com/project/keesverruijt/radar-pi]
* [https://travis-ci.org/opencpn-radar-pi/radar_pi]

_Travis release build fails on Linux_

This is a known issue at this time, the travis code for pushing the artifacts uses an obsolete function that doesn't work. Normal
(builds without tags) work OK.

