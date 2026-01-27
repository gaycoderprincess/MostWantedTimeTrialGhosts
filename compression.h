class CwoeeFileStream {
public:
	uint8_t* pData = nullptr;
	size_t nDataSize = 0;
	size_t nCursor = 0;

	CwoeeFileStream(uint8_t* data, size_t dataSize) : pData(data), nDataSize(dataSize) {}
	~CwoeeFileStream() {
		delete[] pData;
	}
	size_t read(char* out, size_t numBytes) {
		auto bytesRead = std::min(numBytes, nDataSize - nCursor);
		if (!bytesRead) return 0;
		memcpy(out, &pData[nCursor], bytesRead);
		nCursor += bytesRead;
		return bytesRead;
	}
};

std::string ReadStringFromFile(CwoeeFileStream& file) {
	int len = 0;
	file.read((char*)&len, sizeof(len));
	if (len <= 0) return "";

	char* tmp = new char[len];
	file.read(tmp, len);
	std::string str = tmp;
	delete[] tmp;

	return str;
}

CwoeeFileStream* DecompressPB(const std::filesystem::path& filePath) {
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
	auto decompressed = new uint8_t[decompressedSize];
	memset(decompressed, 0, decompressedSize);
	if (!LZDecompress(data, decompressed)) {
		delete[] data;
		return nullptr;
	}
	delete[] data;
	return new CwoeeFileStream(decompressed, decompressedSize);
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