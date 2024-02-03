# OnionClean

OnionClean is a powerful directory scanning tool designed to help users manage their file system more efficiently. It offers detailed insights into directory sizes, allowing for better storage management and cleanup operations.

## Features

- **Directory Size Calculation**: Quickly calculates the size of directories and subdirectories.
- **Progress Bar Display**: Shows a progress bar during scans to indicate completion status.
- **Flexible Output**: Results can be directed to a file for further processing or inspection.
- **Search and Filter**: Allows searching for specific files or directories and filtering by size threshold.
- **Export Functionality**: Provides an option to export the scan results based on search filters or size thresholds.
- **Customizable Scan Depth and Iteration Limits**: Limits can be adjusted to prevent excessive recursion or iteration over directories.

## Installation

To use OnionClean, clone this repository to your local machine. Ensure you have a C compiler (such as GCC) installed on your system.

```
git clone https://github.com/OnionClean/OnionClean.git
cd OnionClean
```

Compile the source code using your C compiler. For example, with GCC:
```
gcc -o OnionClean main.c -lm
```

## Usage

After compiling, you can run OnionClean by executing the compiled binary:
```
./OnionClean
```


Follow the on-screen prompts to navigate through the program's menu. Here are some common operations:

1. **Start Scan**: Begin a new directory scan by specifying the start directory.
2. **Set Output File Path**: Change the default path where scan results are saved.
3. **View Last Scan Results**: Display the results from the most recent scan.
4. **Search Apps**: (MacOS Only) Scan for applications in standard directories.
5. **Exit**: Quit the program.

## Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Contact

Project Link: [https://github.com/OnionClean/OnionClean](https://github.com/OnionClean/OnionClean)

