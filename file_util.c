#include <stdio.h>

#include "mp3_metadata.h"
#include "util.c"


int get_frame_data_len(ID3V2_FRAME_HEADER h) {
    return synchsafeint32ToInt(h.size);
}


/**
 * @brief 
 * 
 * @param new_len 
 * @param f 
 * @param verbose 
 * @return int 
 */
int write_new_len(int new_len, FILE *f, int verbose) {
    char synchsafe_nl[4];
    intToSynchsafeint32(new_len + 1, synchsafe_nl);
    
    if (verbose) {
        for (int i=0; i < 4; i++)
            printf("%d\n",synchsafe_nl[i]);
    }
    
    fwrite(synchsafe_nl, 1, 4, f);

    return new_len;
}



void rewrite_buffer(signed int offset, int remaining_metadata_size, int zero_buf, FILE *f) {
    int buf_size = remaining_metadata_size;
    char *buf = malloc(buf_size);
    fread(buf, 1, buf_size, f);

    if (zero_buf) {
        fseek(f, -1*buf_size, SEEK_CUR);
        char *emp_buf = calloc(buf_size, 1);
        fwrite(emp_buf, 1, buf_size, f);
    }

    fseek(f, -1*(buf_size) + offset, SEEK_CUR);
    fwrite(buf, 1, buf_size, f);

    fseek(f,-1*buf_size,SEEK_CUR);
}



int write_new_data(char *data, ID3V2_FRAME_HEADER header, int remaining_metadata_sz, FILE *f) {
    int frame_data_sz = synchsafeint32ToInt(header.size);
    int new_len = strlen(data);

    if (new_len > frame_data_sz-1) {
        fseek(f, frame_data_sz-1, SEEK_CUR);

        rewrite_buffer(new_len - frame_data_sz, remaining_metadata_sz - frame_data_sz, 0, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        fwrite(data, new_len, 1,f);
        fseek(f, -1 * (new_len + 1), SEEK_CUR);
    } else if (new_len == frame_data_sz - 1) {
        fwrite(data, new_len, 1, f);

        fseek(f,-1 * (new_len + 1),SEEK_CUR);
    } else {
        fseek(f, frame_data_sz-1, SEEK_CUR);

        rewrite_buffer(new_len - frame_data_sz, remaining_metadata_sz - frame_data_sz, 1, f);

        fseek(f, -1 * new_len, SEEK_CUR);
        fwrite(data, new_len, 1,f);
        fseek(f, -1 * (new_len + 1), SEEK_CUR);
    }

    frame_data_sz = new_len + 1;
    fflush(f);

    return frame_data_sz;
}



int read_frame_data(FILE *f, int len_data) {
    char *data = malloc(len_data + 1);
    data[len_data] = '\0';
    
    if (fread(data, 1, len_data, f) != len_data){
        printf("Error occurred reading frame data.\n");
        exit(1);
    }

    free(data);

    return len_data;
}



void print_data(FILE *f, int frames) {
    int bytes_read = 0; 
    char *data;
    char fid_str[5] = {'\0'};

    fseek(f, 10, SEEK_SET);
    
    // Read Final Data
    for (int i = 0; i < frames; i++) {
        ID3V2_FRAME_HEADER frame_header;
        
        if (fread(frame_header.fid, 1, 4, f) != 4) {
            printf("Error occurred reading file identifier.\n");
            exit(1);
        }
        if (fread(frame_header.size, 1, 4, f) != 4) {
            printf("Error occurred tag size.\n");
            exit(1);
        }
        if (fread(frame_header.flags, 1, 2, f) != 2) {
            printf("Error occurred flags.\n");
            exit(1);
        }
        int frame_data_sz = synchsafeint32ToInt(frame_header.size);
        strncpy(fid_str, frame_header.fid, 4);

        printf("FID: %.4s, ", fid_str);
        printf("Size: %d\n", frame_data_sz);

        data = malloc(frame_data_sz+1);
        data[frame_data_sz] = '\0';
        
        if (fread(data, 1, frame_data_sz, f) != frame_data_sz){
            printf("Error occurred reading frame data.\n");
            exit(1);
        }

        if (strncmp(fid_str, "APIC", 4) != 0) {
            // Printing char array with intermediate null chars
            printf("\tData: ");
            for (int i = 0; i < frame_data_sz; i++) {
                if (data[i] != '\0') printf("%c", data[i]);
            }
            printf("\n");
        }  
        else printf("\tData is an image\n");

        free(data);

        bytes_read += 10 + frame_data_sz; 
    }
}
