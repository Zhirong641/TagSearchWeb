# TagSearchWeb

A high-performance C++ web application for searching images by tags. This project provides a fast and efficient way to search through large collections of tagged images using a modern web interface.

## Features

- **Tag-based Image Search**: Search images using multiple tags with support for inclusion and exclusion filters
- **Real-time Tag Suggestions**: Auto-complete functionality for tag input with live filtering
- **Paginated Results**: Display search results with pagination for better performance
- **Image Information**: Hover over images to see detailed tag information and scores
- **High Performance**: Built with C++ backend for fast tag matching and image retrieval
- **Caching Support**: Optional caching of CG list and tags for improved performance

## Architecture

### Backend (C++)
- **HTTP Server**: Built using `httplib` for handling web requests
- **Matrix Library**: Custom matrix implementation for efficient data handling
- **JSON Processing**: Uses `nlohmann/json` for API communication
- **File System Operations**: Efficient file and directory traversal for image and tag management

### Frontend (HTML/JavaScript)
- **Responsive UI**: Modern web interface with real-time search suggestions
- **Image Gallery**: Grid-based image display with hover tooltips
- **AJAX Communication**: Asynchronous requests for smooth user experience

## Data Structure

The application expects the following directory structure:

```
/mnt/shared/data/
├── webp/                           # Image files in WebP format
├── tag/                            # Tag files (one .txt file per image)
├── all_tags_translated_250722.csv # Tag translation mappings
└── cglist_250722.csv              # CG list with metadata
```

### Tag Format
Each image has a corresponding `.txt` file in the `tag/` directory containing:
```
tag_name score
another_tag score
...
```

### CSV Files
- **all_tags_translated_250722.csv**: Contains English and Japanese tag translations
- **cglist_250722.csv**: Contains CG metadata including IDs, titles, and image information

## Building and Running

### Prerequisites
- C++17 compatible compiler
- CMake 3.10 or higher
- Required libraries:
  - httplib
  - nlohmann/json
  - Custom matrix library (included)

### Build Instructions

```bash
# Navigate to the project directory
cd /mnt/shared/source/image_serach

# Compile the application
g++ -std=c++17 -O3 -I./include main.cpp -o image_search_server

# Run the server
./image_search_server
```

### Configuration

The following constants can be modified in `main.cpp`:

```cpp
const std::string image_dir = "/mnt/shared/data/webp";     # Image directory
const std::string tag_dir = "/mnt/shared/data/tag";       # Tag directory
const std::string tag_file = "/mnt/shared/data/all_tags_translated_250722.csv";
const std::string cg_list_file = "/mnt/shared/data/cglist_250722.csv";
const int page_size = 20;                                 # Results per page
const bool cache_cg_info = true;                          # Enable caching
constexpr size_t max_image_count = 10000;                 # Maximum results
```

## API Endpoints

### GET `/`
Serves the main HTML interface.

### GET `/tags?filter=<keyword>`
Returns tag suggestions matching the keyword.

**Response**: JSON array of matching tags
```json
["tag1", "tag2", "tag3"]
```

### POST `/validate`
Validates if provided tags exist in the database.

**Request Body**: Comma-separated tag string
**Response**: "OK" or error message

### POST `/search`
Searches for images matching the provided tags.

**Request Body**:
```json
{
    "tags": "tag1, tag2, -excluded_tag"
}
```

**Response**:
```json
{
    "images": ["path/image1.webp", "path/image2.webp"]
}
```

### GET `/img/<filename>`
Serves image files.

### GET `/image_info?file=<filename>`
Returns detailed tag information for a specific image.

## Tag Search Syntax

- **Basic tags**: `tag1, tag2` - Images must have both tags
- **Exclusion**: `-tag3` - Exclude images with this tag
- **Mixed**: `tag1, tag2, -tag3` - Include tag1 and tag2, but exclude tag3

## Performance Features

- **Caching**: Pre-loads CG list and tags into memory for faster searches
- **Efficient Matching**: Optimized tag matching algorithms
- **Streaming**: Large file operations use streaming for memory efficiency
- **Pagination**: Results are paginated to reduce load times

## Usage Example

1. Start the server:
```bash
./image_search_server
```

2. Open your browser and navigate to `http://localhost:8080`

3. Enter tags in the search box (with auto-complete suggestions)

4. Click "Search" to find matching images

5. Hover over images to see detailed tag information

6. Click on images to view them in full size

## Development

### Adding New Features

The modular design allows for easy extension:

- **New endpoints**: Add to the `svr.Get()` or `svr.Post()` handlers in `main.cpp`
- **UI improvements**: Modify `index.html` and the embedded CSS/JavaScript
- **Performance optimizations**: Enhance the caching mechanisms or search algorithms

### Dependencies

- [httplib](https://github.com/yhirose/cpp-httplib): Single-header HTTP library
- [nlohmann/json](https://github.com/nlohmann/json): JSON library for Modern C++
- Custom matrix library: High-performance matrix operations

## License

This project is open source. See the GitHub repository for license details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues on the [GitHub repository](https://github.com/Zhirong641/TagSearchWeb).

## Links

- [Project Source (GitHub)](https://github.com/Zhirong641/TagSearchWeb)
- [Tag List](https://github.com/Zhirong641/TagSearchWeb/blob/master/data/all_tags_translated_250722.csv)