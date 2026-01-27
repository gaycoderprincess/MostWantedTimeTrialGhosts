uint8_t* DecompressPB(const std::filesystem::path& filePath, uint32_t* outSize) {
	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return nullptr;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);

	struct tHeader {
		uint32_t signature;
		uint8_t _4[4];
		uint32_t decompressedSize;
		uint32_t compressedSize;
	};
	auto header = (tHeader*)data;
	auto decompressedSize = header->decompressedSize;
	*outSize = decompressedSize;
	auto decompressed = new uint8_t[decompressedSize];
	memset(decompressed, 0, decompressedSize);
	if (!LZDecompress(data, decompressed)) {
		delete[] data;
		return nullptr;
	}
	delete[] data;
	return decompressed;
}

bool CompressPB(const std::filesystem::path& filePath) {
	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return false;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);

	auto compressed = new uint8_t[size];
	auto newSize = LZCompress(data, size, compressed);

	auto outFile = std::ofstream(filePath.string() + "2", std::ios::out | std::ios::binary);
	if (!outFile.is_open()) return false;
	outFile.write((char*)compressed, newSize);
	return true;
}

struct IMemBuf: std::streambuf {
	IMemBuf(const char* base, size_t size) {
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};

struct IMemStream: virtual IMemBuf, std::istream {
	IMemStream(const char* mem, size_t size) :
			IMemBuf(mem, size),
			std::istream(static_cast<std::streambuf*>(this))
	{
	}
};

std::string ReadStringFromFile(IMemStream& file) {
	int len = 0;
	file.read((char*)&len, sizeof(len));
	if (len <= 0) return "";

	char* tmp = new char[len];
	file.read(tmp, len);
	std::string str = tmp;
	delete[] tmp;

	return str;
}

/*if (std::filesystem::exists(fileName)) {
	if (CompressPB(fileName)) {
		std::filesystem::remove(fileName);
	}
}

auto newFileName = fileName + "2";
if (!std::filesystem::exists(newFileName)) {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_IN_FRONTEND) {
		WriteLog("No ghost found for " + fileName);
	}
	return;
}

uint32_t size = 0;
auto decompress = DecompressPB(fileName + "2", &size);
if (!decompress) {
	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_IN_FRONTEND) {
		WriteLog("Invalid ghost for " + fileName);
	}
	return;
}

class temp {
public:
	uint8_t* data;

	temp(uint8_t* a) : data(a) {}
	~temp() { delete[] data; }
} dtor(decompress);

auto inFile = IMemStream((char*)decompress, size);*/