#ifndef __VTF_H__
#define __VTF_H__

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <vector>


namespace Vtf {


enum Format {
	FormatNone				= 0xFFFFFFFF,
	FormatRGBA8888			= 0,
	FormatABGR8888			= 1,
	FormatRGB888			= 2,
	FormatBGR888			= 3,
	FormatRGB565			= 4,
	FormatI8				= 5,
	FormatIA88				= 6,
	FormatP8				= 7,
	FormatA8				= 8,
	FormatRGB888_BlueScreen	= 9,
	FormatBGR888_BlueScreen	= 10,
	FormatARGB8888			= 11,
	FormatBGRA8888			= 12,
	FormatDXT1				= 13,
	FormatDXT3				= 14,
	FormatDXT5				= 15,
	FormatBGRX8888			= 16,
	FormatBGR565			= 17,
	FormatBGRX5551			= 18,
	FormatBGRA4444			= 19,
	FormatDXT1_1bitAlpha	= 20,
	FormatBGRA5551			= 21,
	FormatUV88				= 22,
	FormatUVWQ8888			= 23,
	FormatRGBA16161616F		= 24,
	FormatRGBA16161616		= 25,
	FormatUVLX8888			= 26,
};



class Resource
{
public:
	enum Type {
		TypeUnknown	= 0x00000000,
		TypeLowres	= 0x00000001,
		TypeHires	= 0x00000030,
		TypeCRC		= 0x02435243
	};
	
	inline Resource(Type type) : mType(type)
		{}
	virtual inline ~Resource()
		{};
	
	inline Type getType() const
		{return mType;}
	
private:
	Type	mType;
};



class ImageResource : public Resource
{
public:
	ImageResource(Type type) : Resource(type), m_Format(FormatNone), m_Width(0), m_Height(0)
		{}
	
	inline Format format () const
		{return m_Format;}
	
	inline uint16_t width () const
		{return m_Width;}
	
	inline uint16_t height () const
		{return m_Height;}
	
protected:
	Format m_Format;
	uint16_t m_Width;
	uint16_t m_Height;
};


class LowresImageResource : public ImageResource
{
public:
	inline LowresImageResource() : ImageResource(TypeLowres), m_Image(NULL)
		{}
	
	inline ~LowresImageResource()
		{ if (m_Image) delete[] m_Image; }
	
	void read(std::istream& stm, uint32_t offset, Format format,
			uint16_t width, uint16_t height);
	
	void setup(Format format, uint16_t width, uint16_t height);
	void write (std::ostream& stm) const;
	
private:
	uint8_t* m_Image;
};



class HiresImageResource : public ImageResource
{
public:
	HiresImageResource();
	inline ~HiresImageResource()
		{clear();}
	
	void read(std::istream& stm, uint32_t offset, Format format,
			uint16_t width, uint16_t height, uint16_t depth,
			uint8_t mipmaps, uint16_t frames);
	
	inline uint16_t depth()
		{return m_Depth;}
	
	inline uint32_t frameCount()
		{return m_FrameCount;}
	
	inline uint8_t mipmapCount()
		{return m_MipmapCount;}
	
	uint8_t* getImage(uint8_t mipmap, uint16_t frame, uint16_t face, uint16_t slice);
	uint8_t* getImageRGBA(uint8_t mipmap, uint16_t frame, uint16_t face, uint16_t slice);
	
	void clear();
	void setup(Format format, uint16_t width, uint16_t height, uint8_t mipmaps, uint16_t frames,
			uint16_t faces, uint16_t slices);
	void setImage(uint8_t mipmap, uint16_t frame, uint16_t face, uint16_t slice, uint8_t* data);
	bool check();
	void write (std::ostream& stm);
	
protected:
	uint16_t m_Depth;
	uint8_t m_MipmapCount;
	uint16_t m_FrameCount;
	
	typedef std::vector<uint8_t*>	SliceList;
	typedef std::vector<SliceList>	FaceList;
	typedef std::vector<FaceList>	FrameList;
	typedef std::vector<FrameList>	MipmapList;
	MipmapList mImages;
};


class CRCResource : public Resource
{
public:
	inline CRCResource() : Resource(TypeCRC), m_CRC(0)
		{}
	
	inline ~CRCResource()
		{}
	
	inline uint32_t get()
		{return m_CRC;}
	
	inline void set(uint32_t crc)
		{m_CRC = crc;}
	
private:
	uint32_t m_CRC;
};


class File
{
public:
	File();
	~File();
	
	void load(const std::string& fname);
	void load(const char* data, std::size_t length);
	void load(FILE* f);
	void load(std::istream& stm);
	
	void save(const std::string& fname, uint32_t version);
	void save(std::ostream& stm, uint32_t version);
	
	void addResource(Resource* res);
	void delResource(Resource::Type type);
	Resource* findResource(Resource::Type type);
	
private:
	typedef std::vector<Resource*> ResourceList;
	ResourceList mResourceList;
};



class Exception : public std::exception
{
public:
	Exception(const std::string& message) throw ();
	~Exception() throw ();
	
	const char* what() const throw ();

private:
	std::string mMessage;
};


const char* formatToString (Format format);


}

#endif
