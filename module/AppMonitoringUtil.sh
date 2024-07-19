#!/system/bin/sh

# This script is a monitoring app for Encore Tweaks
# Please modify according to your needs and ROM compatibility

# Line with '#' prefix will be not executed

# Grep mCurrentFocus from dumpsys
#dumpsys window displays | grep -E "mCurrentFocus"

# Grep mFocusedApp from dumpsys
dumpsys window displays | grep -E "mFocusedApp"

# Grep mObsecuringWindow from dumpsys
#dumpsys window windows | grep -E "mObscuringWindow"
