from setuptools import setup, find_packages

setup(
    name="raycandle",
    version="2.0.0",
    packages=find_packages(),
    install_requires=[
        "cffi>=1.0.0",
        "numpy",
        "pytz",
        "pandas",
        "typing_extensions",        
    ],
    package_data={
        "raycandle": [
            "libraycandle.so",
            "raycandle.h",
            "assets/*",
        ]
    },
    include_package_data=True,
    zip_safe=False,
)
