#include <stdio.h>

typedef   struct {  
    char         fccID[4];        
    unsigned   long       dwSize;            
    char         fccType[4];    
} wav_header;  

typedef   struct {  
    char         fccID[4];        
    unsigned   long       dwSize;            
    unsigned   short     wFormatTag;    
    unsigned   short     wChannels;  
    unsigned   long       dwSamplesPerSec;  
    unsigned   long       dwAvgBytesPerSec;  
    unsigned   short     wBlockAlign;  
    unsigned   short     uiBitsPerSample;  
} wav_format;

typedef   struct {  
    char         fccID[4];          
    unsigned   long       dwSize;              
} wav_data;  

int pcm2wav(char *input_pcm, char *output_wav)
{
    FILE *pcm_fp = NULL;
    FILE *wav_fp = NULL;
    pcm_fp = fopen(input_pcm, "rb");
    if (NULL == pcm_fp) {
        printf("input pcm file open error:%s\n", input_pcm);
        return -1;
    }
    wav_fp = fopen(output_wav, "wb+");
    if (NULL == wav_fp) {
        printf("output wav file open error:%s\n", output_wav);
        return -1;
    }
    wav_header header;
    wav_format format;
    wav_data data;
    unsigned short mData;
    //populate wav_header area;
    strncpy(header.fccID, "RIFF", 4);     
    strncpy(header.fccType, "WAVE", 4);  
    fseek(wav_fp, sizeof(wav_header), 1); //jump over size of wav_header; 
    if(ferror(wav_fp))
        printf("error\n");
    //populate wav_format area;
    strncpy(format.fccID, "fmt ", 4);
    format.dwSamplesPerSec = 44100;  
    format.dwAvgBytesPerSec = format.dwSamplesPerSec * 2;
    format.uiBitsPerSample = 16;  
    format.dwSize = 16;  
    format.wBlockAlign = 2;  
    format.wChannels = 1;  
    format.wFormatTag = 1;
    fwrite(&format, sizeof(wav_format), 1, wav_fp);
    
    //populate wav_data area;
    data.dwSize = 0; //data.dwsize is set to 0, it will be populated below 
    strncpy(data.fccID, "data", 4);  
    fseek(wav_fp, sizeof(wav_data), 1);
    fread(&mData, sizeof(unsigned short), 1, pcm_fp);
    while(!feof(pcm_fp))   {  
        data.dwSize += 2;
        fwrite(&mData, sizeof(unsigned short), 1, wav_fp);  
        fread(&mData, sizeof(unsigned short), 1, pcm_fp); //从.pcm中读入数据  
    }
    fclose(pcm_fp);
    
    header.dwSize= 44 + data.dwSize; 
    rewind(wav_fp); 
    fwrite(&header, sizeof(wav_header), 1, wav_fp);  
    fseek(wav_fp, sizeof(wav_format), 1);
    fwrite(&data, sizeof(wav_data), 1, wav_fp);
    fclose(wav_fp);
    return 0;

}

int main (int argc, char *argv[])
{
	if (argc != 3){
		printf("usage:");
		printf("app pcm_file wav_file\n");
		return 1;
	}
    
    pcm2wav(argv[1], argv[2]);
    return 0;
}
