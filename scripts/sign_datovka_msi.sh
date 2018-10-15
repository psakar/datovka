#!/usr/bin/env sh

# Instruction how to use the script:
# 1. Create dir "source-packages" in the root of script location if not exists.
# 2. Copy all msi packages to be sign into "source-packages" dir.
# 3. Run the script.
# 4. Signed msi packages are generated in the "msi-signed" dir.

SIGN_CERT_ID="CZ.NIC, z.s.p.o."

MSI_SOURCE_DIR="source-packages"
MSI_SIGNED_DIR="msi-signed"

rm -r $MSI_SIGNED_DIR
mkdir $MSI_SIGNED_DIR

msiFiles=$(find $MSI_SOURCE_DIR -name "*.msi")

for msi in $msiFiles; do

	# Sign Datovka msi package
	echo
	echo ---Sign Datovka msi package---
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" $msi

	# Verify Datovka msi signature
	echo
	echo ---Verify dmg signature---
	codesign -dv --verbose=4 $msi
	echo

	cp ${msi} ${MSI_SIGNED_DIR}
done
