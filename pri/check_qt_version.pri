
defineTest(sufficientQtVersion) {
	# Required Qt versions
	required_major = $$1
	required_minor = $$2
	advised_ninor = $$3
	advised_patch = $$4

	lessThan(QT_MAJOR_VERSION, $${required_major}) {
		error(Qt version $${required_major}.$${required_minor} is required.)
		return(false)
	}

	isEqual(QT_MAJOR_VERSION, $${required_major}) {
		lessThan(QT_MINOR_VERSION, $${required_minor}) {
			error(Qt version $${required_major}.$${required_minor} is required.)
			return(false)
		}

		lessThan(QT_MINOR_VERSION, $${advised_ninor}) {
			warning(Qt version at least $${required_major}.$${advised_ninor}.$${advised_patch} is suggested.)
		} else {
			isEqual(QT_MINOR_VERSION, $${advised_ninor}) {
				lessThan(QT_PATCH_VERSION, $${advised_patch}) {
					warning(Qt version at least $${required_major}.$${advised_ninor}.$${advised_patch} is suggested.)
				}
			}
		}
	} else {
		warning(The current Qt version $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION} may not work.)
	}
	return(true)
}
