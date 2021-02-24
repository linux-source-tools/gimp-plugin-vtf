#include <squish.h>
#include <math.h>
#include <fstream>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include "vtf.h"


namespace Vtf {


/* flags */
#define VTF_FLAG_POINTSAMPLE			0x00000001
#define VTF_FLAG_TRILINEAR				0x00000002
#define VTF_FLAG_CLAMPS					0x00000004
#define VTF_FLAG_CLAMPT					0x00000008
#define VTF_FLAG_ANISOTROPIC			0x00000010
#define VTF_FLAG_HINT_DXT5				0x00000020
#define VTF_FLAG_PWL_CORRECTED			0x00000040
#define VTF_FLAG_NORMAL					0x00000080
#define VTF_FLAG_NOMIP					0x00000100
#define VTF_FLAG_NOLOD					0x00000200
#define VTF_FLAG_ALL_MIPS				0x00000400
#define VTF_FLAG_PROCEDURAL				0x00000800
#define VTF_FLAG_ONEBITALPHA			0x00001000
#define VTF_FLAG_EIGHTBITALPHA			0x00002000
#define VTF_FLAG_ENVMAP					0x00004000
#define VTF_FLAG_RENDERTARGET			0x00008000
#define VTF_FLAG_DEPTHRENDERTARGET		0x00010000
#define VTF_FLAG_NODEBUGOVERRIDE		0x00020000
#define VTF_FLAG_SINGLECOPY				0x00040000
#define VTF_FLAG_PRE_SRGB				0x00080000
#define VTF_FLAG_NODEPTHBUFFER			0x00800000
#define VTF_FLAG_CLAMPU					0x02000000
#define VTF_FLAG_VERTEXTEXTURE			0x04000000
#define VTF_FLAG_SSBUMP					0x08000000
#define VTF_FLAG_BORDER					0x20000000


#define VTF_VERSION(hdr, major, minor)	\
	(hdr.version[0] >= major && hdr.version[1] >= minor)

#define IS_POWER_OF_TWO(t) (t > 0 && !(t & (t - 1)))


struct Header {
	uint32_t	magic;				/* file signature ("VTF\0") */
	uint32_t	version[2];			/* version[0].version[1] (currently 7.2) */
	uint32_t	headerSize;			/* size of the header struct (16 byte aligned; currently 80 bytes) */
	
	uint16_t	width;				/* Width of the largest mipmap in pixels. Must be a power of 2 */
	uint16_t	height;				/* Height of the largest mipmap in pixels. Must be a power of 2 */
	uint32_t	flags;				/* VTF flags */
	uint16_t	frameCount;				/* Number of frames, if animated (1 for no animation) */
	uint16_t	firstFrame;		/* First frame in animation (0 based) */
	uint8_t		padding0[4];
	
	float		reflectivity[3];	/* reflectivity vector */
	uint8_t		padding1[4];
	
	/* version 7.0 */
	float		bumpmapScale;		/* Bumpmap scale */
	Format		format;				/* High resolution image format */
	uint8_t		mipmapCount;		/* Number of mipmaps */
	Format		lowresFormat;		/* Low resolution image format (always DXT1) */
	uint8_t		lowresWidth;		/* Low resolution image width */
	uint8_t		lowresHeight;		/* Low resolution image height */
	
	/* version 7.2 */
	uint16_t	depth;				/* Depth of the largest mipmap in pixels */
	
	/* version 7.3 */
	uint8_t		padding2[3];
	uint32_t	resourceCount;
	uint32_t	padding[2];
} __attribute__((packed));


struct HeaderResource {
	Resource::Type type;
	uint32_t offset;
} __attribute__((packed));



inline uint16_t
calcMipmapSize (uint16_t size, uint8_t mipmap)
{
	uint32_t ret = size / pow(2, mipmap);
	return ret > 0 ? ret : 1;
}


static uint32_t
getImageLength (Format format, uint16_t width, uint16_t height)
{
	uint32_t npixels = width * height;
	
	switch (format) {
		case FormatRGBA8888:
		case FormatABGR8888:
		case FormatARGB8888:
		case FormatBGRA8888:
		case FormatBGRX8888:
			return npixels * 4;
		case FormatRGB888:
		case FormatBGR888:
		case FormatRGB888_BlueScreen:
		case FormatBGR888_BlueScreen:
			return npixels * 3;
		case FormatRGB565:
		case FormatBGR565:
		case FormatBGRX5551:
		case FormatBGRA4444:
		case FormatBGRA5551:
			return npixels * 2;
		case FormatDXT1:
			return npixels < 16 ? 8 : npixels / 2;
		case FormatDXT3:
		case FormatDXT5:
			return npixels < 16 ? 16 : npixels;
		default:
			return 0;
	}
}

const char *
formatToString (Format format)
{
	switch (format) {
		case FormatNone:				return "None";
		case FormatRGBA8888:			return "RGBA8888";
		case FormatABGR8888:			return "ABGR8888";
		case FormatRGB888:				return "RGB888";	
		case FormatBGR888:				return "BGR888";	
		case FormatRGB565:				return "RGB565";	
		case FormatI8:					return "I8";	
		case FormatIA88:				return "IA88";
		case FormatP8:					return "P8";
		case FormatA8:					return "A8";
		case FormatRGB888_BlueScreen:	return "RGB888_BlueScreen";
		case FormatBGR888_BlueScreen:	return "BGR888_BlueScreen";
		case FormatARGB8888:			return "ARGB8888";
		case FormatBGRA8888:			return "BGRA8888";
		case FormatDXT1:				return "DXT1";
		case FormatDXT3:				return "DXT3";
		case FormatDXT5:				return "DXT5";
		case FormatBGRX8888:			return "BGRX8888";
		case FormatBGR565:				return "BGR565";
		case FormatBGRX5551:			return "BGRX5551";
		case FormatBGRA4444:			return "BGRA4444";
		case FormatDXT1_1bitAlpha:		return "DXT1_1bitAlpha";
		case FormatBGRA5551:			return "BGRA5551";
		case FormatUV88:				return "UV88";
		case FormatUVWQ8888:			return "UVWQ8888";
		case FormatRGBA16161616F:		return "RGBA16161616F";
		case FormatRGBA16161616:		return "RGBA16161616";
		case FormatUVLX8888:			return "UVLX8888";
		default:						return "Unknown";
	}
}



Exception::Exception(const std::string& message) throw ()
	: mMessage(message)
{
}

Exception::~Exception() throw ()
{
}

const char* Exception::what() const throw ()
{
	return mMessage.c_str();
}



/* Vtf::LowresImageResource */
void LowresImageResource::read(std::istream& stm, uint32_t offset,
		Format format, uint16_t width, uint16_t height)
{
	if (m_Image)
		delete[] m_Image;
	
	uint32_t length = getImageLength (format, width, height);
	m_Image = new uint8_t[length];
	stm.seekg (offset);
	stm.read ((std::istream::char_type*) m_Image, length);
	if (stm.fail ())
		throw Exception ("Could not read Low-resolution image");
}


void LowresImageResource::setup(Format format, uint16_t width, uint16_t height)
{
	m_Format = format;
	m_Width = width;
	m_Height = height;
}


void LowresImageResource::write (std::ostream& stm) const
{
	uint32_t length = getImageLength (m_Format, m_Width, m_Height);
	stm.write ((std::istream::char_type*) m_Image, length);
	if (stm.fail ())
		throw Exception ("Could not write Low-resolution image");
}



/* Vtf::HiresImage */
HiresImageResource::HiresImageResource()
	: ImageResource(TypeHires), m_Depth(0), m_MipmapCount(0), m_FrameCount(0)
{
}


void HiresImageResource::clear()
{
	for (MipmapList::iterator mm = mImages.begin(); mm != mImages.end(); ++mm)
		for (FrameList::iterator fr = mm->begin(); fr != mm->end(); ++fr)
			for (FaceList::iterator fc = fr->begin(); fc != fr->end(); ++fc)
				for (SliceList::iterator sl = fc->begin(); sl != fc->end(); ++sl)
					delete[] *sl;
}


void HiresImageResource::read(std::istream& stm, uint32_t offset, Format format,
			uint16_t width, uint16_t height, uint16_t depth,
			uint8_t mipmaps, uint16_t frames)
{
	setup(format, width, height, mipmaps, frames, 1, depth);
	
	for (int mm = 0; mm < mipmaps; mm++) {
		uint16_t w = calcMipmapSize(width, mipmaps - mm - 1);
		uint16_t h = calcMipmapSize(height, mipmaps - mm - 1);
		for (int fr = 0; fr < frames; fr++) {
			for (int fc = 0; fc < 1; fc++) {
				for (int sl = 0; sl < depth; sl++) {
					uint32_t len = getImageLength(format, w, h);
					uint8_t* data = new uint8_t[len];
					stm.read((std::istream::char_type*) data, len);
					if (stm.fail()) {
						delete data;
						throw Exception("Could not read high-resolution image");
					}
					setImage(mipmaps - mm - 1, fr, fc, sl, data);
				}
			}
		}
	}
}


uint8_t* HiresImageResource::getImage(uint8_t mipmap, uint16_t frame,
		uint16_t face, uint16_t slice)
{
	assert(mipmap < m_MipmapCount);
	assert(frame < m_FrameCount);
	assert(face < 1);
	assert(slice < m_Depth);
	
	return mImages[mipmap][frame][face][slice];
}


uint8_t* HiresImageResource::getImageRGBA(uint8_t mipmap, uint16_t frame,
		uint16_t face, uint16_t slice)
{
	/* let's clear it up. SIZE is dimension / resolution. LENGTH is data length */
	uint16_t img_width = calcMipmapSize (m_Width, mipmap);
	uint16_t img_height = calcMipmapSize (m_Height, mipmap);
	uint8_t* img_data = getImage(mipmap, frame, face, slice);
	uint32_t rgba_length = getImageLength(FormatRGBA8888, img_width, img_height);
	uint8_t *rgba_data = new uint8_t[rgba_length];
	uint32_t c, i;
	
	switch (m_Format) {
	case FormatRGBA8888:
		memcpy (rgba_data, img_data, rgba_length);
		break;
	case FormatABGR8888:
		c = img_width * img_height;
		for (i = 0; i < c; i++) {
			rgba_data[i * 4 + 0] = img_data[i * 4 + 3];
			rgba_data[i * 4 + 1] = img_data[i * 4 + 2];
			rgba_data[i * 4 + 2] = img_data[i * 4 + 1];
			rgba_data[i * 4 + 3] = img_data[i * 4 + 0];
		}
		break;
	case FormatRGB888:
		c = img_width * img_height;
		for (i = 0; i < c; i++) {
			rgba_data[i * 4 + 0] = img_data[i * 3 + 0];
			rgba_data[i * 4 + 1] = img_data[i * 3 + 1];
			rgba_data[i * 4 + 2] = img_data[i * 3 + 2];
			rgba_data[i * 4 + 3] = 255;
		}		
		break;
	case FormatBGR888:
		c = img_width * img_height;
		for (i = 0; i < c; i++) {
			rgba_data[i * 4 + 0] = img_data[i * 3 + 2];
			rgba_data[i * 4 + 1] = img_data[i * 3 + 1];
			rgba_data[i * 4 + 2] = img_data[i * 3 + 0];
			rgba_data[i * 4 + 3] = 255;
		}
		break;
	case FormatARGB8888:
		c = img_width * img_height;
		for (i = 0; i < c; i++) {
			rgba_data[i * 4 + 0] = img_data[i * 4 + 3];
			rgba_data[i * 4 + 1] = img_data[i * 4 + 0];
			rgba_data[i * 4 + 2] = img_data[i * 4 + 1];
			rgba_data[i * 4 + 3] = img_data[i * 4 + 2];
		}
		break;
	case FormatBGRA8888:
		c = img_width * img_height;
		for (i = 0; i < c; i++) {
			rgba_data[i * 4 + 0] = img_data[i * 4 + 2];
			rgba_data[i * 4 + 1] = img_data[i * 4 + 1];
			rgba_data[i * 4 + 2] = img_data[i * 4 + 0];
			rgba_data[i * 4 + 3] = img_data[i * 4 + 3];
		}
		break;
	case FormatDXT1:
		squish::DecompressImage(rgba_data, img_width, img_height, img_data, squish::kDxt1);
		break;
	case FormatDXT3:
		squish::DecompressImage(rgba_data, img_width, img_height, img_data, squish::kDxt3);
		break;
	case FormatDXT5:
		squish::DecompressImage(rgba_data, img_width, img_height, img_data, squish::kDxt5);
		break;
	default:
		delete[] rgba_data;
		return NULL;
	}
	
	return rgba_data;
}


void HiresImageResource::setup(Format format, uint16_t width, uint16_t height,
		uint8_t mipmaps, uint16_t frames, uint16_t faces, uint16_t slices)
{
	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_Depth = slices;
	m_MipmapCount = mipmaps;
	m_FrameCount = frames;
	
	mImages.resize(mipmaps);
	for (MipmapList::iterator mm = mImages.begin(); mm != mImages.end(); ++mm) {
		mm->resize(frames);
		for (FrameList::iterator fr = mm->begin(); fr != mm->end(); ++fr) {
			fr->resize(faces);
			for (FaceList::iterator fc = fr->begin(); fc != fr->end(); ++fc) {
				fc->resize(slices);
				for (SliceList::iterator sl = fc->begin(); sl != fc->end(); ++sl)
					*sl = NULL;
			}
		}
	}
}


bool HiresImageResource::check()
{
	for (MipmapList::iterator mm = mImages.begin(); mm != mImages.end(); ++mm)
		for (FrameList::iterator fr = mm->begin(); fr != mm->end(); ++fr)
			for (FaceList::iterator fc = fr->begin(); fc != fr->end(); ++fc)
				for (SliceList::iterator sl = fc->begin(); sl != fc->end(); ++sl)
					if (!*sl)
						return false;
	return true;
}


void HiresImageResource::setImage(uint8_t mipmap, uint16_t frame, uint16_t face,
		uint16_t slice, uint8_t* data)
{
	mImages[mipmap][frame][face][slice] = data;
}


void HiresImageResource::write (std::ostream& stm)
{
	for (int mm = 0; mm < m_MipmapCount; mm++) {
		uint16_t w = calcMipmapSize(m_Width, m_MipmapCount - mm - 1);
		uint16_t h = calcMipmapSize(m_Height, m_MipmapCount - mm - 1);
		for (int fr = 0; fr < m_FrameCount; fr++) {
			for (int fc = 0; fc < 1; fc++) {
				for (int sl = 0; sl < m_Depth; sl++) {
					uint32_t len = getImageLength(m_Format, w, h);
					uint8_t* data = getImage (mm, fr, fc, sl);
					stm.write((std::ostream::char_type*) data, len);
					if (stm.fail()) {
						delete data;
						throw Exception("Could not write high-resolution image");
					}
				}
			}
		}
	}
}



File::File()
{
}


File::~File()
{
	for (ResourceList::iterator i = mResourceList.begin(); i != mResourceList.end(); ++i)
		delete *i;
}


void File::load(const std::string& fname)
{
	std::ifstream stm;
	stm.open(fname.c_str(), std::ios::binary);
	load(stm);
	stm.close();
}


void File::load(const char* data, std::size_t length)
{
	using namespace boost::iostreams;
	stream<array_source> stm(data, length);
	load(stm);
}


void File::load(FILE* f)
{
	using namespace boost::iostreams;
	stream<file_descriptor_source> stm(fileno(f), never_close_handle);
	load(stm);
}


void File::load(std::istream& stm)
{
	Header hdr;
	
	/* read magic, version and header size */
	stm.read((char*) &hdr, 16);
	if (stm.fail())
		throw Exception("Header is too small");
	
	if (hdr.magic != 0x00465456)
		throw Exception("Not a VTF file");
	
	if (hdr.version[0] != 7)
		throw Exception("Unknown version");
	
	stm.seekg(0);
	stm.read((char*) &hdr, sizeof(hdr));
	if (stm.fail())
		throw Exception("Header is too small");
	
	if (!(IS_POWER_OF_TWO(hdr.width) && IS_POWER_OF_TWO(hdr.height)))
		throw Exception("Dimensions of the image are not power of 2");
	
	if (hdr.mipmapCount == 0)
		throw Exception("Number of mipmap images equals 0");
	
	if (!(IS_POWER_OF_TWO(hdr.lowresWidth) && IS_POWER_OF_TWO(hdr.lowresHeight)))
		throw Exception("Lowres image dimensions are not power of 2");
	
	if (hdr.version[0] >= 7 && hdr.version[1] >= 2) {
		if (hdr.depth == 0)
			throw Exception("Texture depth equals 0");
	} else {
		hdr.depth = 1;
	}
	
	if (hdr.version[0] >= 7 && hdr.version[1] >= 3) {
		HeaderResource* rsrc = new HeaderResource[hdr.resourceCount];
		stm.read((char*) rsrc, sizeof(rsrc) * hdr.resourceCount);
		if (stm.fail()) {
			delete rsrc;
			throw Exception("Header is too small");
		}
		
		for (uint32_t i = 0; i < hdr.resourceCount; i++) {
			switch (rsrc[i].type) {
			case Resource::TypeLowres: {
				LowresImageResource* res = new LowresImageResource;
				res->read(stm, rsrc[i].offset, hdr.lowresFormat,
						hdr.lowresWidth, hdr.lowresHeight);
				addResource(res);
				} break;
			case Resource::TypeHires: {
				HiresImageResource* res = new HiresImageResource;
				res->read(stm, rsrc[i].offset, hdr.format, hdr.width, hdr.height,
						hdr.depth, hdr.mipmapCount, hdr.frameCount);
				addResource(res);
				}break;
			case Resource::TypeCRC: {
				CRCResource* res = new CRCResource;
				res->set(rsrc[i].offset);
				addResource(res);
				} break;
			default:
				throw Exception("Unknown resource type");
			}
		}
		
		delete[] rsrc;
	} else {
		/* This version does not support resources, but we add them anyway.
			First read lowres image, if needed. */
		if (hdr.lowresFormat != FormatNone) {
			LowresImageResource* res = new LowresImageResource;
			res->read(stm, hdr.headerSize, hdr.lowresFormat,
					hdr.lowresWidth, hdr.lowresHeight);
			addResource(res);
		}
		/* then read actual image */
		HiresImageResource* res = new HiresImageResource;
		res->read(stm, stm.tellg(), hdr.format, hdr.width, hdr.height,
				hdr.depth, hdr.mipmapCount, hdr.frameCount);
		addResource(res);
	}
}


void File::save(const std::string& fname, uint32_t version)
{
	std::ofstream stm(fname.c_str(), std::ios::binary);
	save(stm, version);
	stm.close();
}


void File::save(std::ostream& stm, uint32_t version)
{
	Header hdr;
	memset (&hdr, 0, sizeof (hdr));
	
	hdr.magic = 0x00465456;
	hdr.version[0] = 7;
	hdr.version[1] = version;
	
	HiresImageResource* hires = (HiresImageResource*) findResource(Resource::TypeHires);
	if (!hires)
		throw Exception("High-resolution image is not provided");
	
	LowresImageResource* lowres = (LowresImageResource*) findResource(Resource::TypeLowres);
	
	hdr.width = hires->width();
	hdr.height = hires->height();
	hdr.frameCount = hires->frameCount();
	hdr.format = hires->format();
	hdr.mipmapCount = hires->mipmapCount();
	
	if (lowres) {
		hdr.lowresFormat = lowres->format();
		hdr.lowresWidth = lowres->width();
		hdr.lowresHeight = lowres->height();
	}
	
	/* base header size */
	switch (version) {
		case 0:  hdr.headerSize = 80; break;
		case 2:  hdr.headerSize = 80;  break; /* + */
		case 3:  hdr.headerSize = 104; break;
		case 4:  hdr.headerSize = 104; break; /* + */
		default:
			throw Exception ("Unsupported version");
	}
	
	/*  */
	stm.write((char*) &hdr, hdr.headerSize);
	
	if (hdr.version[1] >= 3) {
		
	}
	
	if (lowres)
		lowres->write (stm);
	hires->write (stm);
}


int
calc_mipmap_count (int width, int height)
{
	int size = std::max(width, height);
	int count = 1;
	
	while (size == 1) {
		size /= 2;
		count++;
	}
	
	return count;
}



void File::addResource(Resource* res)
{
	mResourceList.push_back(res);
}


Resource* File::findResource(Resource::Type type)
{
	for (ResourceList::iterator i = mResourceList.begin(); i != mResourceList.end(); ++i)
		if ((*i)->getType() == type)
			return (*i);
	return NULL;
}


void File::delResource(Resource::Type type)
{
	/* TODO */
}

	
}
