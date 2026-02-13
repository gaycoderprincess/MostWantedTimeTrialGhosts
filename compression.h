class CwoeeIStream {
public:
	uint8_t* pData = nullptr;
	size_t nDataSize = 0;
	size_t nCursor = 0;

	CwoeeIStream(uint8_t* data, size_t dataSize) : pData(data), nDataSize(dataSize) {}
	~CwoeeIStream() {
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

class CwoeeOStream {
public:
	std::vector<char> aData;

	void write(const char* in, size_t numBytes) {
		for (int i = 0; i < numBytes; i++) {
			aData.push_back(in[i]);
		}
	}
};

std::string ReadStringFromFile(CwoeeIStream& file) {
	int len = 0;
	file.read((char*)&len, sizeof(len));
	if (len <= 0) return "";

	char* tmp = new char[len];
	file.read(tmp, len);
	std::string str = tmp;
	delete[] tmp;

	return str;
}
void WriteStringToFile(CwoeeOStream& file, const char* string) {
	int len  = lstrlen(string) + 1;
	file.write((char*)&len, sizeof(len));
	file.write(string, len);
}

// raw data

CwoeeIStream* OpenRawPB(const std::filesystem::path& filePath) {
	if (!std::filesystem::exists(filePath)) return nullptr;

	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return nullptr;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);
	return new CwoeeIStream(data, size);
}

bool WriteRawPB(CwoeeOStream* file, const std::filesystem::path& filePath) {
	auto outFile = std::ofstream(filePath.string(), std::ios::out | std::ios::binary);
	if (!outFile.is_open()) return false;
	outFile.write((char*)&file->aData[0], file->aData.size());
	return true;
}

// huff compression

CwoeeIStream* OpenCompressedPB(const std::filesystem::path& filePath) {
	if (!std::filesystem::exists(filePath)) return nullptr;

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
	return new CwoeeIStream(decompressed, decompressedSize);
}

bool CompressPB(const std::filesystem::path& filePath) {
	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return false;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);

	auto compressed = new uint8_t[size];
	auto newSize = HUFFCompress(data, size, compressed);
	delete[] data;

	auto outFile = std::ofstream(filePath.string() + "2", std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		delete[] compressed;
		return false;
	}
	outFile.write((char*)compressed, newSize);
	delete[] compressed;
	return true;
}

bool WriteCompressedPB(CwoeeOStream* file, const std::filesystem::path& filePath) {
	auto compressed = new uint8_t[file->aData.size()];
	auto newSize = HUFFCompress((uint8_t*)&file->aData[0], file->aData.size(), compressed);

	auto outFile = std::ofstream(filePath.string() + "2", std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		delete[] compressed;
		return false;
	}
	outFile.write((char*)compressed, newSize);
	delete[] compressed;
	return true;
}

// basic cipher method

void EncryptGhostData(uint8_t* in, size_t size, uint8_t* out) {
	uint8_t tmp = 0;
	for (size_t i = 0; i < size; i++) {
		auto origValue = in[i];
		out[i] = in[i] ^ tmp;
		tmp += origValue + 0x10;
	}
}

void DecryptGhostData(uint8_t* in, size_t size, uint8_t* out) {
	uint8_t tmp = 0;
	for (size_t i = 0; i < size; i++) {
		out[i] = in[i] ^ tmp;
		tmp += out[i] + 0x10;
	}
}

CwoeeIStream* OpenEncryptedPB(const std::filesystem::path& filePath) {
	if (!std::filesystem::exists(filePath)) return nullptr;

	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return nullptr;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);
	DecryptGhostData(data, size, data);

	return new CwoeeIStream(data, size);
}

bool EncryptPB(const std::filesystem::path& filePath) {
	auto size = std::filesystem::file_size(filePath);
	auto inFile = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!inFile.is_open()) return false;

	auto data = new uint8_t[size];
	inFile.read((char*)data, size);

	auto encrypted = new uint8_t[size];
	EncryptGhostData(data, size, encrypted);
	delete[] data;

	auto outFile = std::ofstream(filePath.string() + "3", std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		delete[] encrypted;
		return false;
	}
	outFile.write((char*)encrypted, size);
	delete[] encrypted;
	return true;
}

bool WriteEncryptedPB(CwoeeOStream* file, const std::filesystem::path& filePath) {
	auto encrypted = new uint8_t[file->aData.size()];
	EncryptGhostData((uint8_t*)&file->aData[0], file->aData.size(), encrypted);

	auto outFile = std::ofstream(filePath.string() + "3", std::ios::out | std::ios::binary);
	if (!outFile.is_open()) {
		delete[] encrypted;
		return false;
	}
	outFile.write((char*)encrypted, file->aData.size());
	delete[] encrypted;
	return true;
}