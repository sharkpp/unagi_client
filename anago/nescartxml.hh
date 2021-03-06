#pragma once
class wxXmlDocument;
class CartridgeHash;
class RomDb{
private:
	wxXmlDocument *m_document;
	CartridgeHash *m_hash;
public:
	RomDb(wxString file);
	RomDb(void);
	bool Generate(void);
	void Search(unsigned long crc, wxTextCtrl *log) const;
	~RomDb();
};
