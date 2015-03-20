
TEST_TARGETS = \
	test_01 \
	test_02 \
	test_03 \
	test_04 \
	test_05 \
	test_06 \
	test_07

SHARED_OBJECTS = crypto.o helper.o log_c_stub.o

CPPFLAGS = -I../ -D_BSD_SOURCE -D_POSIX_SOURCE

CFLAGS = -g -O0 -std=c99 -Wall -Wextra -pedantic -fstack-protector

# OpenSSL
CPPFLAGS += $(shell pkg-config --cflags openssl)
LDFLAGS += -lcrypto

.PHONY: all clean help run

all: $(TEST_TARGETS)

help:
	@echo Targets: all clean help run


# Disable implicit rule.
%: %.c


%.o: %.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)


crypto.o: ../src/crypto/crypto.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)


%: %.o $(SHARED_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)


run: $(TEST_TARGETS)
	@for prog in $^; do \
		./$${prog} 2>/dev/null && echo $${prog} OK || echo $${prog} FAIL; \
	done

clean:
	rm -rf *.o $(TEST_TARGETS)
