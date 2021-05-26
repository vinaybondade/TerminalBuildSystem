#ifndef __LINUX_SYSFS_MACRO_H_
#define __LINUX_SYSFS_MACRO_H_
/* ----------------------------
 *           includes
 * ----------------------------
 */

/* ----------------------------
 *      registers macros
 * ----------------------------
 */
#define DEFINE_RW_REG_ATTR_AND_FUNC(name, regOffset) \
static ssize_t name##_store(struct device *dev, struct device_attribute *attr,\
			 const char *buf, size_t count) \
{ \
	return sysfs_reg_write(dev, buf, count, regOffset); \
} \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, char *buf) \
{ \
	return sysfs_reg_read(dev, buf, regOffset); \
} \
static DEVICE_ATTR_RW(name);


#define DEFINE_RO_REG_ATTR_AND_FUNC(name, regOffset) \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, char *buf) \
{ \
	return sysfs_reg_read(dev, buf, regOffset); \
} \
static DEVICE_ATTR_RO(name);


#define DEFINE_WO_REG_ATTR_AND_FUNC(name, regOffset) \
static ssize_t name##_store(struct device *dev, struct device_attribute *attr, \
			 const char *buf, size_t count) \
{ \
	return sysfs_reg_write(dev, buf, count, regOffset); \
} \
static DEVICE_ATTR_WO(name);

/* ----------------------------
 *      statistic macros
 * ----------------------------
 */
#define DEFINE_RW_STAT_ATTR_AND_FUNC(name, stat) \
static ssize_t name##_store(struct device *dev, struct device_attribute *attr,\
			 const char *buf, size_t count) \
{ \
	return sysfs_stat_write(dev, buf, count, stat); \
} \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, char *buf) \
{ \
	return sysfs_stat_read(dev, buf, stat); \
} \
static DEVICE_ATTR_RW(name);


#define DEFINE_RO_STAT_ATTR_AND_FUNC(name, stat) \
static ssize_t name##_show(struct device *dev, \
			struct device_attribute *attr, char *buf) \
{ \
	return sysfs_stat_read(dev, buf, stat); \
} \
static DEVICE_ATTR_RO(name);


#define DEFINE_WO_STAT_ATTR_AND_FUNC(name, stat) \
static ssize_t name##_store(struct device *dev, struct device_attribute *attr, \
			 const char *buf, size_t count) \
{ \
	return sysfs_stat_write(dev, buf, count, stat); \
} \
static DEVICE_ATTR_WO(name);

/* ----------------------------
 *      General macros
 * ----------------------------
 */
#define NAME_CONCAT(a,b) a##b
#define DEV_ATTR(attrName) NAME_CONCAT(dev_attr_, attrName)

	
#endif /* __LINUX_SYSFS_MACRO_H_ */
