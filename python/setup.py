from distutils.core import setup
import glob

setup(name='LibrePilot UAVTalk',
      version='1.0',
      description='LibrePilot UAVTalk',
      url='http://www.librepilot.org',
      packages=['librepilot', 'librepilot.uavtalk', 'librepilot.uavobjects'],
     )
