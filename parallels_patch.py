# Parallels Desktop built-in USB device patcher
# ---------------------------------------------

# Running this script on a Parallels Desktop installation will allow you to
# connect devices to a VM that are considered to be "built-in" by the OS X
# System Information utility.

# This is particularly useful on Hackintosh setups where every USB device
# is marked as built-in...

# sudo python3 parallels_patch.py

paths = (
    'MacOS/prl_client_app',
    'MacOS/Parallels Service.app/Contents/MacOS/prl_disp_service',
    'MacOS/Parallels VM.app/Contents/MacOS/prl_vm_app',
    'Applications/Parallels Technical Data Reporter.app/Contents/MacOS/prl_technical_data_reporter',
    'Frameworks/libPrlGui.3.dylib',
    'Frameworks/ParallelsVirtualizationSDK.framework/Versions/7/ParallelsVirtualizationSDK'
)

# 14.1.3-45485: bgrep 410F95C6EB1266904489E7E8 to find
# 15.1.0-47107: bgrep 410F95C6EB1266904489EFE8 to find

basepath = '/Applications/Parallels Desktop.app/Contents'

for path in paths:
    fullpath = '%s/%s' % (basepath, path)
    with open(fullpath, 'rb') as f:
        blob = f.read()

    # 14.1.3-45485
    a = b'\x41\x0F\x95\xC6\xEB\x12\x66\x90\x44\x89\xE7\xE8'
    b = b'\x45\x31\xF6\x90\xEB\x12\x66\x90\x44\x89\xE7\xE8'

    # 15.1.0-47107
    a = b'\x41\x0F\x95\xC6\xEB\x12\x66\x90\x44\x89\xEF\xE8'
    b = b'\x45\x31\xF6\x90\xEB\x12\x66\x90\x44\x89\xEF\xE8'

    blob = blob.replace(a, b)
    with open(fullpath, 'wb') as f:
        f.write(blob)
