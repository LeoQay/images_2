#include <cstdio>
#include <cstdlib>
#include <cstring>



bool ThiningDIBSkeleton (unsigned char * lpDIBBits, int lWidth, int lHeight)
{
	long i;
	long j;
	long lLength;
 
	 unsigned char deletemark [256] = {
		0,0,0,0,0,0,0,1,	0,0,1,1,0,0,1,1,
		0,0,0,0,0,0,0,0,	0,0,1,1,1,0,1,1,
		0,0,0,0,0,0,0,0,	1,0,0,0,1,0,1,1,
		0,0,0,0,0,0,0,0,	1,0,1,1,1,0,1,1,
		0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,	1,0,0,0,1,0,1,1,
		1,0,0,0,0,0,0,0,	1,0,1,1,1,0,1,1,
		0,0,1,1,0,0,1,1,	0,0,0,1,0,0,1,1,
		0,0,0,0,0,0,0,0,	0,0,0,1,0,0,1,1,
		1,1,0,1,0,0,0,1,	0,0,0,0,0,0,0,0,
		1,1,0,1,0,0,0,1,	1,1,0,0,1,0,0,0,
		0,1,1,1,0,0,1,1,	0,0,0,1,0,0,1,1,
		0,0,0,0,0,0,0,0,	0,0,0,0,0,1,1,1,
		1,1,1,1,0,0,1,1,	1,1,0,0,1,1,0,0,
		1,1,1,1,0,0,1,1,	1,1,0,0,1,1,0,0
	 };
 
	unsigned char p0, p1, p2, p3, p4, p5, p6, p7;
	char unsigned *pmid, *pmidtemp; 
	unsigned char sum;
	bool bStart = true;
	lLength = lWidth * lHeight;
	unsigned char * pTemp = new unsigned char [sizeof (unsigned char) * lWidth * lHeight];
	
	//    P0 P1 P2
	//    P7    P3
	//    P6 P5 P4
 
	while(bStart)
	{
		bStart = false;
 
		pmid = (unsigned char *) lpDIBBits + lWidth + 1;
		memset(pTemp,  0, lLength);
		pmidtemp = (unsigned char *) pTemp + lWidth + 1;
		
		for(i = 1; i < lHeight -1; i++)     
		{
			for(j = 1; j < lWidth - 1; j++)
			{
				if (*pmid == 0)
				{
					pmid++;
					pmidtemp++;
					continue;
				}
				p3 = *(pmid + 1);
				p2 = *(pmid + 1 - lWidth);
				p1 = *(pmid - lWidth);
				p0 = *(pmid - lWidth -1);
				p7 = *(pmid - 1);
				p6 = *(pmid + lWidth - 1);
				p5 = *(pmid + lWidth);
				p4 = *(pmid + lWidth + 1);				
				
				sum = p0 & p1 & p2 & p3 & p4 & p5 & p6 & p7;
				if(sum == 0) *pmidtemp = 1;
 
				pmid++;
				pmidtemp++;
			}

			pmid++;
			pmid++;
			pmidtemp++;
			pmidtemp++;
		}
		
		pmid = (unsigned char *) lpDIBBits + lWidth + 1;
		pmidtemp = (unsigned char *) pTemp + lWidth + 1;
 
		 for (i = 1; i < lHeight -1; i ++)
		{
			for(j = 1; j < lWidth - 1; j++)
			{
				 if (* pmidtemp == 0)
				{
					pmid++;
					pmidtemp++;
					continue;
				}
 
				p3 = *(pmid + 1);
				p2 = *(pmid + 1 - lWidth);
				p1 = *(pmid - lWidth);
				p0 = *(pmid - lWidth -1);
				p7 = *(pmid - 1);
				p6 = *(pmid + lWidth - 1);
				p5 = *(pmid + lWidth);
				p4 = *(pmid + lWidth + 1);
				
				p1 *= 2;
				p2 *= 4;
				p3 *= 8;
				p4 *= 16;
				p5 *= 32;
				p6 *= 64;
				p7 *= 128;
 
				sum = p0 | p1 | p2 | p3 | p4 | p5 | p6 | p7;
				if(deletemark[sum] == 1)
				{
					*pmid = 0;
					 bStart = true;
				}
				pmid++;
				pmidtemp++;
			}
 
			pmid++;
			pmid++;
			pmidtemp++;
			pmidtemp++;
		}
	}

	delete [] pTemp;
	return true;
}


struct Image
{
	unsigned char * data;
	int64_t * shape;
	int64_t n_dims;
	int64_t total_len;
};


void bin_image(Image & image, unsigned char val)
{
	for (int64_t i = 0; i < image.total_len; i++)
	{
		if (image.data[i] != 0) image.data[i] = val;
	}
}


Image read_image(const char * filename)
{
	FILE * file = fopen(filename, "rb");

	Image image;

	fread(&image.n_dims, sizeof(int64_t), 1, file);

	image.shape = new int64_t[image.n_dims];
	fread(image.shape, sizeof(int64_t), image.n_dims, file);

	image.total_len = 1;
	for (int i = 0; i < image.n_dims; i++) image.total_len *= image.shape[i];

	image.data = new unsigned char[image.total_len];
	fread(image.data, sizeof(unsigned char), image.total_len, file);

	fclose(file);
	return image;
}


void write_image(const Image & image, const char * filename)
{
	FILE * file = fopen(filename, "wb");

	fwrite((void *) &image.n_dims, sizeof(int64_t), 1, file);
	fwrite((void *) image.shape, sizeof(int64_t), image.n_dims, file);
	fwrite((void *) image.data, sizeof(unsigned char), image.total_len, file);

	fclose(file);
}


Image new_image_like(const Image & image)
{
	Image new_image;
	new_image.data = new unsigned char[image.total_len];
	new_image.shape = new int64_t[image.n_dims];
	memcpy(new_image.shape, image.shape, image.n_dims * sizeof(int64_t));
	new_image.n_dims = image.n_dims;
	new_image.total_len = image.total_len;

	return new_image;
}


void delete_image(Image & image)
{
	delete [] image.data;
	delete [] image.shape;
	image.data = nullptr;
	image.shape = nullptr;
	image.n_dims = 0;
	image.total_len = 0;
}


int main(int argc, char ** argv)
{
	const char * src_filename = argv[1];
	const char * dest_filename = argv[2];

	Image image = read_image(src_filename);

	bin_image(image, 1);

    ThiningDIBSkeleton(image.data, image.shape[1], image.shape[0]);

	bin_image(image, 255);

	write_image(image, dest_filename);

	delete_image(image);

	return 0;
}