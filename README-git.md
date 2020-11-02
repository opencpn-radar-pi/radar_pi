## Experimental Metadata Git Push  README

This plugin has experimental support for pushing metadata to a remote git
repository. The goal is to make the process for creating pull requests
against OpenCPN/plugins simpler.

When enabled, all metadata files are replicated to a remote plugins repo
defined by the user. This repo can be used to make a PR against
OpenCPN/plugins without further actions.

To enable this support:
  - Make sure you have a clone of https://github.com/OpenCPN/plugins.git
    on github.
  - Create a branch called *auto* in this clone, something like:

        $ cd plugins
        $ git checkout -b auto master
        $ git push origin auto:auto

  - Run the script *ci/new-credentials* in your plugin repo. Script will ask
    for your ssh url to your plugins repo and a secret password.
  - Dont forget the password...
  - Register the new public key (displayed by new-credentials) as a deployment
    key with write permissions on your github plugins repo
    (in *Settings | Deploy keys*)
  - Script will create an encrypted private + a public key in the *ci*
    directory. Commit and push these:

        $ git add ci/*.enc ci/*.pub
        $ git commit -m "Add git deployment key."
        $ git push origin master:master

   - Set environment variables in all builders e. g., circleci and appveyor:
      + GIT_KEY_PASSWORD:  Password entered to new-credentials.
      + GIT_REPO: The ssh-based url to your plugins repository, the same as
        entered to new-credentials.

When the build is run, the git-push part will

  - Clone your plugins repository
  - Add the new metadata file
  - Commit and push the change to the auto branch in your repo.

By default the changes are pushed to the 'auto' branch. The branch name can be
tweaked using the GIT_BRANCH environment variable in the builder.

### Making a Pull Request

The builds will make commits in the auto branch, one for each build. To make
a PR from these do something like:

    $ git checkout auto
    $ git rebase master
    $ git checkout master
    $ git merge --squash auto

The rebase operation should not be strictly necessary, but will pick up
possible errors if the metadata has been changed by other means. The --squash
option lumps the too many commits in the auto branch (one for each build) to
a single one, keeping the master branch tidy.

### Security

The private ssh key created by new-credentials is encrypted using a standard DES
alghorithm. There is probably some room to crack this given the fact that part of
ciphertext is known. The encryption would be stronger if the header and trailer
of the key wasn't encrypted.

That said, given the context this should be reasonably safe. At least, a separate
ssh key is used for this purpose, a key which could be easily revoked.


### Bugs:

Probably plenty.
