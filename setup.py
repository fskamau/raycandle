from setuptools import setup, find_packages

setup(
    name="raycandle",
    version="0.1.0",
    author_email="kamstefin@gmail.com",
    packages=find_packages(),
    install_requires=[
        "cffi>=1.0.0",
        "numpy",
        "pandas",
        "typing_extensions",
    ],
    package_data={
        "raycandle": [
            "libraycandle.so.1",
            "raycandle_for_cffi.h",
            "fonts/*",
        ]
    },
    include_package_data=True,
    zip_safe=False,
)
