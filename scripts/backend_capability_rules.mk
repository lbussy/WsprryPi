# Shared compile-time backend-capability generation rules.

BACKEND_CAPABILITIES_PYTHON ?= python3
BACKEND_CAPABILITIES_NONBUILD_GOALS := clean help macros backend-capabilities-regression-test backend-capabilities-make-regression-test
BACKEND_CAPABILITIES_BUILD_GOALS := $(filter-out $(BACKEND_CAPABILITIES_NONBUILD_GOALS),$(MAKECMDGOALS))

ifneq ($(strip $(BACKEND_CAPABILITIES_BUILD_GOALS)),)
BACKEND_CAPABILITIES_ENABLED := 1
else ifeq ($(strip $(MAKECMDGOALS)),)
BACKEND_CAPABILITIES_ENABLED := 1
endif

ifeq ($(BACKEND_CAPABILITIES_ENABLED),1)

ifneq ($(strip $(BACKENDS)),$(DEFAULT_BACKENDS))
$(error non-default BACKENDS profiles are not enabled yet; supported foundation value is BACKENDS=$(DEFAULT_BACKENDS))
endif

ifeq ($(BUILD_METADATA_INTROSPECTION),)
# Treat the generated C++ header as an included Makefile so GNU Make restarts
# freshness evaluation when the generator repairs stale capability content.
-include $(BACKEND_CAPABILITIES_HEADER)

.PHONY: FORCE_BACKEND_CAPABILITIES
FORCE_BACKEND_CAPABILITIES:

$(BACKEND_CAPABILITIES_HEADER): FORCE_BACKEND_CAPABILITIES $(BACKEND_CAPABILITIES_GENERATOR)
	$(Q)$(BACKEND_CAPABILITIES_PYTHON) $(BACKEND_CAPABILITIES_GENERATOR) \
		--backends "$(BACKENDS)" --output $@
else
BACKEND_CAPABILITIES_CHECK_STATUS := $(strip $(shell $(BACKEND_CAPABILITIES_PYTHON) $(BACKEND_CAPABILITIES_GENERATOR) --backends "$(BACKENDS)" --output $(BACKEND_CAPABILITIES_HEADER) --check >/dev/null 2>&1; printf '%s' $$?))

ifneq ($(filter 0 3,$(BACKEND_CAPABILITIES_CHECK_STATUS)),)
else
$(error backend capabilities read-only check failed (status $(BACKEND_CAPABILITIES_CHECK_STATUS)))
endif

ifneq ($(BACKEND_CAPABILITIES_CHECK_STATUS),0)
$(BACKEND_CAPABILITIES_HEADER):
	$(Q)$(BACKEND_CAPABILITIES_PYTHON) $(BACKEND_CAPABILITIES_GENERATOR) \
		--backends "$(BACKENDS)" --output $@
endif
endif

endif
