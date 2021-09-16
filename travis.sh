#!/bin/bash

# install artifacts uploader
curl -sL https://raw.githubusercontent.com/travis-ci/artifacts/master/install | bash

# build the PDFium branch matching the current tag
[[ $TRAVIS_BRANCH == chromium/* ]] && export PDFium_BRANCH=$TRAVIS_BRANCH

./build.sh

[ -n "$TARGET_CPU" ] || ./test.sh

$HOME/bin/artifacts upload \
	--target-paths "artifacts/$TRAVIS_BUILD_NUMBER/$TRAVIS_JOB_NUMBER"\
	$(ls pdfium*.tgz | tr "\n" ":")
