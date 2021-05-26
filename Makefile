prj=$(PWD)
include $(prj)/dev.mk

all:	build

build:	test_env kernel u-boot fsbl get_bins
	@for i in $(apps); do \
		$(MAKE) dev=$(dev) -C apps/$$i all; \
		if [ $$? -ne 0 ]; \
		then \
			echo "Component: $$i build failed. Stop!"; \
			exit 1; \
		fi \
	done;

install: build install_fsbl
	@for i in $(apps); do \
		$(MAKE) dev=$(dev) -C apps/$$i install; \
	done;

clean: 
	@for i in $(apps); do \
		$(MAKE) dev=$(dev) -C apps/$$i clean; \
	done

distclean:
	@for i in $(apps); do \
		$(MAKE) dev=$(dev) -C apps/$$i distclean; \
	done;

image:
	$(MAKE) build
	$(MAKE) install
	$(prj)/3rdparty/$(dev)/create_img.sh $(dev);

deploy:
	$(MAKE) build
	$(MAKE) install
	$(MAKE) image

