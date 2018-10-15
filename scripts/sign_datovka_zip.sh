#!/usr/bin/env sh

# Instruction how to use the script:
# 1. Create dir "source-packages" in the root of script location if not exists.
# 2. Copy all zip packages to be sign into "source-packages" dir.
# 3. Run the script.
# 4. Signed zip packages are generated in the "zip-signed" dir.

SIGN_CERT_ID="CZ.NIC, z.s.p.o."

ZIP_SOURCE_DIR="source-packages"
WORK_DIR="work-space"
ZIP_SIGNED_DIR="zip-signed"

rm -r $ZIP_SIGNED_DIR
mkdir $ZIP_SIGNED_DIR

zipFiles=$(find $ZIP_SOURCE_DIR -name "*.zip")

for zip in $zipFiles; do

	mkdir $WORK_DIR
	unzip ${zip} -d $WORK_DIR

	# Sign all application binaries/libraries
	echo
	echo ---Sign all Datovka binaries---
	dllFiles=$(find $WORK_DIR -name "*.dll")
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" $dllFiles
	echo
	echo ---Sign all Datovka executables---
	exeFiles=$(find $WORK_DIR -name "*.exe")
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" $exeFiles

	# Verify application signature
	echo
	echo ---Verify application signature---
	codesign -dv --verbose=4 $exeFiles
	echo

	# ZIP signed application
	signZip=${zip//${ZIP_SOURCE_DIR}/${ZIP_SIGNED_DIR}}
	cd $WORK_DIR
	zip -r -X ../$signZip .
	cd ..
	rm -r $WORK_DIR
done
