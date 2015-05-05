PROJECT_NAME = mmculib
RELEASE_NUM = $(shell cat VERSION)

RELEASE_NAME = $(PROJECT_NAME)-$(RELEASE_NUM)
RELEASE_TAG = $(shell echo $(PROJECT_NAME)-$(RELEASE_NUM) | tr . _ | tr 'a-z' 'A-Z')

all:
	# Nothing to do

.PHONY: docs
docs: 
	(cd doc; doxygen doxygen.config)

.PHONY: release

release: 
	cvs tag $(RELEASE_TAG)
	cvs export -r $(RELEASE_TAG) -d $(RELEASE_NAME) mph/projects/ece/micro/$(PROJECT_NAME)
	zip -r $(RELEASE_NAME).zip $(RELEASE_NAME)/*
	tar cvfhz $(RELEASE_NAME).tgz $(RELEASE_NAME)
	rm -r $(RELEASE_NAME)

version:
	@echo $(RELEASE_NUM)



