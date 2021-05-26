################################################################################
#
# hostapd
#
################################################################################

HOSTAPD_VERSION = 2.9
HOSTAPD_SITE = http://w1.fi/releases
HOSTAPD_SUBDIR = hostapd
HOSTAPD_CONFIG = $(HOSTAPD_DIR)/$(HOSTAPD_SUBDIR)/.config
HOSTAPD_DEPENDENCIES = host-pkgconf libnl
HOSTAPD_CFLAGS = $(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/libnl3/
HOSTAPD_LICENSE = BSD-3-Clause
HOSTAPD_LICENSE_FILES = README
HOSTAPD_CONFIG_SET =

HOSTAPD_CONFIG_ENABLE = \
	CONFIG_HS20 \
	CONFIG_IEEE80211AC \
	CONFIG_IEEE80211N \
	CONFIG_IEEE80211R \
	CONFIG_INTERNAL_LIBTOMMATH \
	CONFIG_INTERWORKING \
	CONFIG_LIBNL32

HOSTAPD_CONFIG_DISABLE =

# libnl-3 needs -lm (for rint) and -lpthread if linking statically
# And library order matters hence stick -lnl-3 first since it's appended
# in the hostapd Makefiles as in LIBS+=-lnl-3 ... thus failing
ifeq ($(BR2_STATIC_LIBS),y)
HOSTAPD_LIBS += -lnl-3 -lm -lpthread
endif

# Try to use openssl if it's already available
ifeq ($(BR2_PACKAGE_LIBOPENSSL),y)
HOSTAPD_DEPENDENCIES += libopenssl
HOSTAPD_LIBS += $(if $(BR2_STATIC_LIBS),-lcrypto -lz)
HOSTAPD_CONFIG_EDITS += 's/\#\(CONFIG_TLS=openssl\)/\1/'
else
HOSTAPD_CONFIG_DISABLE += CONFIG_EAP_PWD
HOSTAPD_CONFIG_EDITS += 's/\#\(CONFIG_TLS=\).*/\1internal/'
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_ACS),y)
HOSTAPD_CONFIG_ENABLE += CONFIG_ACS
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_EAP),y)
HOSTAPD_CONFIG_ENABLE += \
	CONFIG_EAP \
	CONFIG_RADIUS_SERVER

# Enable both TLS v1.1 (CONFIG_TLSV11) and v1.2 (CONFIG_TLSV12)
HOSTAPD_CONFIG_ENABLE += CONFIG_TLSV1
else
HOSTAPD_CONFIG_DISABLE += CONFIG_EAP
HOSTAPD_CONFIG_ENABLE += \
	CONFIG_NO_ACCOUNTING \
	CONFIG_NO_RADIUS
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_WPS),y)
HOSTAPD_CONFIG_ENABLE += CONFIG_WPS
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_VLAN),)
HOSTAPD_CONFIG_ENABLE += CONFIG_NO_VLAN
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_VLAN_DYNAMIC),y)
HOSTAPD_CONFIG_ENABLE += CONFIG_FULL_DYNAMIC_VLAN
endif

ifeq ($(BR2_PACKAGE_HOSTAPD_VLAN_NETLINK),y)
HOSTAPD_CONFIG_ENABLE += CONFIG_VLAN_NETLINK
endif

define HOSTAPD_CONFIGURE_CMDS
	cp -v $(@D)/../../../../buildroot-src/package/hostapd/defconfig $(HOSTAPD_CONFIG)
endef

define HOSTAPD_BUILD_CMDS
	$(TARGET_MAKE_ENV) CFLAGS="$(HOSTAPD_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" LIBS="$(HOSTAPD_LIBS)" \
		$(MAKE) CC="$(TARGET_CC)" -C $(@D)/$(HOSTAPD_SUBDIR)
endef

define HOSTAPD_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 -D $(@D)/$(HOSTAPD_SUBDIR)/hostapd \
		$(TARGET_DIR)/usr/sbin/hostapd
	$(INSTALL) -m 0755 -D $(@D)/$(HOSTAPD_SUBDIR)/hostapd_cli \
		$(TARGET_DIR)/usr/bin/hostapd_cli
	$(INSTALL) -m 0644 -D $(@D)/$(HOSTAPD_SUBDIR)/hostapd.conf \
		$(TARGET_DIR)/etc/hostapd.conf
endef

$(eval $(generic-package))
