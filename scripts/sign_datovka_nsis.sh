#!/usr/bin/env sh

# Instruction how to use the script:
# 1. Create dir "source-packages" in the root of script location if not exists.
# 2. Copy all exe installers to be sign into "source-packages" dir.
# 3. Run the script.
# 4. Signed exe installers are generated in the "nsis-signed" dir.

SIGN_CERT_ID="CZ.NIC, z.s.p.o."

NSIS_SOURCE_DIR="source-packages"
NSIS_SIGNED_DIR="nsis-signed"

rm -r $NSIS_SIGNED_DIR
mkdir $NSIS_SIGNED_DIR

nsisFiles=$(find $NSIS_SOURCE_DIR -name "*.exe")

for nsis in $nsisFiles; do

	# Sign Datovka nsis installer
	echo
	echo ---Sign Datovka nsis installer---
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" $nsis

	# Verify Datovka msi signature
	echo
	echo ---Verify nsis signature---
	codesign -dv --verbose=4 $nsis
	echo

	cp ${nsis} ${NSIS_SIGNED_DIR}
done
