###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
###########################################################################
C_STANDARD   = gnu99
CPP_STANDARD = gnu++11

SOURCES_CPP  = TestDataStructures.cpp
SOURCES_CPP += IdentityTest.cpp
SOURCES_CPP += SchedulerTest.cpp
SOURCES_CPP += BufferPipeTest.cpp

LOCAL_CXX_FLAGS  = $(CXXFLAGS) -D_GNU_SOURCE

LIBS  = -L$(OUTPUT_PATH) -L$(BUILD_ROOT)/lib -lstdc++ -lm
LIBS += -lmanuvr -lextras -lpthread

###########################################################################
# Option conditionals
###########################################################################
ifeq ($(SECURE),1)
	LIBS += $(OUTPUT_PATH)/libmbedtls.a
	LIBS += $(OUTPUT_PATH)/libmbedx509.a
	LIBS += $(OUTPUT_PATH)/libmbedcrypto.a
	SOURCES_CPP   += CryptoTest.cpp
endif

TESTS  = $(SOURCES_CPP:.cpp=)
COV_FILES = $(SOURCES_CPP:.cpp=.gcda) $(SOURCES_CPP:.cpp=.gcno)

###########################################################################
# Parameter unification and make targets
###########################################################################

.PHONY: all


all: buildtests
	@echo 'Beginning test execution...'
	@$(foreach test,$(TESTS),./$(test);)

buildtests: $(TESTS)
	@echo 'Built tests:  $(TESTS)'

% : %.cpp
	@echo 'LIBS:  $(LIBS)'
	$(CXX) -static -o $@ $< $(CXXFLAGS) -std=$(CPP_STANDARD) $(LIBS)

clean:
	rm -f $(TESTS) CryptoTest $(COV_FILES) *.gcno *.gcda
