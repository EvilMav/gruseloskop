import setuptools

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup_args = {
    "name": "gruseloskop",
    "version": "0.2.0",
    "author": "Ilya Elenskiy",
    "author_email": "i.elenskiy@tu-bs.de",
    "license": "GPLv3",
    "description": "Primitive USB oscilloscope using Arduino Uno and python",
    "long_description": long_description,
    "long_description_content_type": "text/markdown",
    "url": "https://github.com/EvilMav/gruseloskop",
    "packages": setuptools.find_packages()#["gruseloskop"],
}

if __name__ == "__main__":
    setuptools.setup(
        **setup_args,
        classifiers=[
            "Topic :: Scientific/Engineering",
            "Development Status :: 4 - Beta",
        ],
        python_requires=">=3.7",
        scripts=["bin/gruseloskop"],
        install_requires=["pyserial", "numpy", "pyqtgraph", "PySide2"],
        include_package_data=True,
    )