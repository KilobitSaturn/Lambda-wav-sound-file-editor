#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <windows.h>
#include <ctype.h>
#include <math.h>

typedef enum {false, true} bool;

struct wavefmt_header
{
    char chunkID[4];
    uint32_t chunksize;
    char format[4], subchunk1ID[4];
    uint32_t subchunk1size;
    uint16_t audioformat, numchannels;
    uint32_t samplerate, byterate;
    uint16_t blockalign, bitspersample;
    char subchunk2ID[4];
    uint32_t subchunk2size;
};

struct wavefmt_length
{
    int time, min, s;
};

struct wavefmt
{
    FILE *fp;
    char title[256], filename[256];
    int numsamples;
    struct wavefmt_header header;
    struct wavefmt_length length;
};

float absr(float num)
{
    if(num < 0) return -num;
    else return num;
}

int absi(int num)
{
    if(num < 0) return -num;
    else return num;
}

int wavegetlength(struct wavefmt_length *lth, bool reverse)
{
    switch(reverse)
    {
    case false:
        lth->min =(int) lth->time/60;
        lth->s = lth->time%60;
        break;
    case true:
        lth->time = 60*lth->min + lth->s;
        break;
    default:
        return -1;
        break;
    }
    return reverse;
}

void wavelog(struct wavefmt *wav)
{
    FILE *log = fopen("wavelog.txt", "w+t");
    fprintf(log, "Filename: \"%s\"\n", wav->filename);
    fprintf(log, "%.4s: %d (%.4s)\n", wav->header.chunkID, wav->header.chunksize, wav->header.format);
    fprintf(log, "%.4s: %d bytes |%d| %d channel(s), %d Hz, %d byte/s, %d byte/sample, %d bits/channel-sample\n", wav->header.subchunk1ID, wav->header.subchunk1size, wav->header.audioformat, wav->header.numchannels, wav->header.samplerate, wav->header.byterate, wav->header.blockalign, wav->header.bitspersample);
    fprintf(log, "%.4s: %d bytes\n", wav->header.subchunk2ID, wav->header.subchunk2size);
    fprintf(log, "length: %d:%02d (%d s)\n", wav->length.min, wav->length.s, wav->length.time);
    fprintf(log, "numsamples: %d\n", wav->numsamples);
    fclose(log);
}

void wavelogprintf(void)
{
    FILE *log = fopen("wavelog.txt", "rt");
    char ch;
    ch = getc(log);
    while(!feof(log))
    {
        putchar(ch);
        ch = getc(log);
    }
    fclose(log);
}

int waveintro(struct wavefmt *wav)
{
    sprintf(wav->filename, "%s.wav", wav->title);
    //printf("%s\n", wav->filename);
    FILE *arq = fopen(wav->filename, "rb");
    if(arq == NULL)
    {
        fclose(arq);
        return 0;
    }
    else
    {
        fread(&wav->header, sizeof (struct wavefmt_header), 1, arq);
        wav->numsamples = wav->header.subchunk2size/wav->header.blockalign;
        wav->length.time = wav->header.subchunk2size/wav->header.byterate;
        wavegetlength(&wav->length, false);
        wavelog(wav);
        fclose(arq);
        return 1;
    }
}

int wavebps(struct wavefmt *ext, const int bps, struct wavefmt *org)
{
    if(bps == org->header.bitspersample) return 0;
    else if(bps != 8 && bps != 16) return -1;
    else
    {
        ext->header = org->header;

        //determinando cabe alho de destino
        printf("\tGerando arquivo...\n");
        ext->header.bitspersample = bps;
        ext->header.blockalign = ext->header.numchannels*ext->header.bitspersample/8;
        ext->header.byterate = ext->header.samplerate*ext->header.blockalign;
        ext->header.subchunk2size = org->header.subchunk2size/org->header.blockalign*ext->header.blockalign;
        ext->header.chunksize = 20 + ext->header.subchunk1size + ext->header.subchunk2size;

        org->fp = fopen(org->filename, "rb");
        fseek(org->fp, sizeof (struct wavefmt_header), SEEK_SET);
        int16_t u, v;

        switch(bps)
        {
        case 8:
            //determinando arquivo de destino
            sprintf(ext->title, "%s {8}", org->title);
            sprintf(ext->filename, "%s.wav", ext->title);
            ext->fp = fopen(ext->filename, "wb");
            fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);

            //escrevendo o arquivo
            for(unsigned int aux = 0; aux <= org->header.subchunk2size; aux+=org->header.bitspersample/8)
            {
                fread(&u, org->header.bitspersample/8, 1, org->fp);
                v =(int) u/256 + 128;
                fwrite(&v, ext->header.bitspersample/8, 1, ext->fp);
            }
            break;
        case 16:
            //determinando arquivo de destino
            sprintf(ext->title, "%s {16}", org->title);
            sprintf(ext->filename, "%s.wav", ext->title);
            ext->fp = fopen(ext->filename, "wb");
            fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);

            //escrevendo o arquivo
            for(unsigned int aux = 0; aux <= org->header.subchunk2size; aux+=org->header.bitspersample/8)
            {
                fread(&u, org->header.bitspersample/8, 1, org->fp);
                v =(int) (u-128)*256;
                fwrite(&v, ext->header.bitspersample/8, 1, ext->fp);
            }
            break;
        }
        fclose(org->fp), fclose(ext->fp);
        return bps;
    }
}

int wavechan(struct wavefmt *ext, const int nchan, struct wavefmt *org)
{
    if(nchan == org->header.numchannels) return 0;
    else if(nchan != 1 && nchan != 2) return -1;
    else
    {
        ext->header = org->header;
        ext->header.numchannels =(uint16_t) nchan;

        //determinando cabe alho de destino
        printf("\tGerando arquivo...\n");
        ext->header.blockalign = ext->header.numchannels*ext->header.bitspersample/8;
        ext->header.byterate = ext->header.samplerate*ext->header.blockalign;
        ext->header.subchunk2size = org->header.subchunk2size/org->header.blockalign*ext->header.blockalign;
        ext->header.chunksize = 20 + ext->header.subchunk1size + ext->header.subchunk2size;

        org->fp = fopen(org->filename, "rb");
        fseek(org->fp, sizeof (struct wavefmt_header), SEEK_SET);
        int16_t u[2], v;

        switch(nchan)
        {
        case 1:
            sprintf(ext->title, "%s (mono)", org->title);
            sprintf(ext->filename, "%s.wav", ext->title);
            ext->fp = fopen(ext->filename, "wb");
            fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);

            for(unsigned int aux = 0; aux <= org->numsamples; aux++)
            {
                fread(&u, org->header.bitspersample/8, org->header.numchannels, org->fp);
                if(org->header.bitspersample == 8) v =(uint8_t) u[0]/2 + u[1]/2;
                else if(org->header.bitspersample == 16) v =(int16_t) u[0]/2 + u[1]/2;
                fwrite(&v, ext->header.bitspersample/8, ext->header.numchannels, ext->fp);
            }
            break;
        case 2:
            sprintf(ext->title, "%s (stereo)", org->title);
            sprintf(ext->filename, "%s.wav", ext->title);
            ext->fp = fopen(ext->filename, "wb");
            fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);

            for(unsigned int aux = 0; aux <= org->numsamples; aux++)
            {
                fread(&u[0], org->header.bitspersample/8, org->header.numchannels, org->fp);
                v =(int) u[0];
                fwrite(&v, ext->header.bitspersample/8, ext->header.numchannels, ext->fp);
            }
            break;
        }
        fclose(org->fp), fclose(ext->fp);
        return nchan;
    }
}

int wavepatch(struct wavefmt *ext, const int to, const int t, struct wavefmt *org)
{
    if(to < 0 || t < 0) return 0;
    else if(to >= t) return -1;
    else if(to >= org->length.time || t > org->length.time) return -2;
    else
    {
        ext->header = org->header;

        //Arrumando o arquivo de destino
        printf("\tGerando arquivo...\n");
        sprintf(ext->title, "%s [%d, %d]", org->title, to, t);
        sprintf(ext->filename, "%s.wav", ext->title);

        const int tsize[2] = {to*org->header.byterate, t*org->header.byterate};
        ext->header.subchunk2size = tsize[1] - tsize[0];
        ext->header.chunksize = 20 + ext->header.subchunk1size + ext->header.subchunk2size;

        ext->fp = fopen(ext->filename, "wb");
        fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);
        org->fp = fopen(org->filename, "rb");
        fseek(org->fp, (sizeof (struct wavefmt_header)) + tsize[0], SEEK_SET);

        int16_t w[org->header.numchannels];
        for(int aux = tsize[0]/org->header.blockalign; aux <= org->numsamples; aux++)
        {
            fread(&w, org->header.bitspersample/8, org->header.numchannels, org->fp);
            fwrite(&w, ext->header.bitspersample/8, ext->header.numchannels, ext->fp);
        }
        fclose(org->fp), fclose(ext->fp);
        return 1;
    }
}

int wavemod(struct wavefmt *ext, const float amp, struct wavefmt *org)
{
    if(amp <= 0) return 0;
    else
    {
        ext->header = org->header;
        sprintf(ext->title, "%s (a%.02f)", org->title, amp);
        sprintf(ext->filename, "%s.wav", ext->title);

        int16_t max, min, w;
        switch(org->header.bitspersample)
        {
        case 16:
            max = 0, min = 0;
            break;
        default:
            return -1;
        }

        org->fp = fopen(org->filename, "rb");
        fseek(org->fp, sizeof (struct wavefmt_header), SEEK_SET);
        for(int aux = 0; aux <= org->header.subchunk2size; aux+=org->header.bitspersample/8)
        {
            fread(&w, org->header.bitspersample/8, 1, org->fp);
            if(max < w) max = w;
            if(min > w) min = w;
        }

        if(absi(min) > max) max = absi(min);
        if(max*amp > 0x7fff) return -2;

        fseek(org->fp, sizeof (struct wavefmt_header), SEEK_SET);

        printf("\tGerando arquivo...\n");
        ext->fp = fopen(ext->filename, "wb");
        fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);
        int16_t u, v;
        for(int aux = 0; aux <= org->header.subchunk2size; aux+=org->header.bitspersample/8)
        {
            fread(&u, org->header.bitspersample/8, 1, org->fp);
            v =(int16_t) u*amp;
            fwrite(&v, ext->header.bitspersample/8, 1, ext->fp);
        }
        fclose(org->fp), fclose(ext->fp);
        return 1;
    }
}

int wavespeed(struct wavefmt *ext, const float v, struct wavefmt *org)
{
    if(v == 0) return 0;
    else
    {
        ext->header = org->header;

        printf("\tGerando arquivo...\n");
        sprintf(ext->title, "%s (v%.02f)", org->title, v);
        sprintf(ext->filename, "%s.wav", ext->title);

        org->fp = fopen(org->filename, "rb");
        ext->fp = fopen(ext->filename, "wb");
        ext->header.subchunk2size =(uint32_t) ext->header.subchunk2size/absr(v);
        ext->header.chunksize = 20 + ext->header.subchunk1size + ext->header.subchunk2size;
        fwrite(&ext->header, sizeof (struct wavefmt_header), 1, ext->fp);
        int16_t w[org->header.numchannels];

        if(v < 0)
        {
            struct wavefmt temp;
            temp.fp = fopen("tempreverse.wav", "wb");
            fwrite(&org->header, sizeof(struct wavefmt_header), 1, temp.fp);
            fseek(org->fp, (sizeof(struct wavefmt_header)) + org->header.subchunk2size - org->header.blockalign, SEEK_SET);
            for(int aux = 0; aux <= org->numsamples; aux++)
            {
                fread(&w, org->header.bitspersample/8, org->header.numchannels, org->fp);
                fwrite(&w, org->header.bitspersample/8, org->header.numchannels, temp.fp);
                fseek(org->fp, -2*org->header.blockalign, SEEK_CUR);
            }
            fclose(temp.fp);
            fclose(org->fp);
            org->fp = fopen("tempreverse.wav", "rb");
        }

        fseek(org->fp, sizeof (struct wavefmt_header), SEEK_SET);
        for(double aux = 0; aux <= org->numsamples; aux+=absr(v))
        {
            if(floor(aux) != floor(aux-absr(v)))
            {
                fread(&w, org->header.bitspersample/8, org->header.numchannels, org->fp);
                fwrite(&w, ext->header.bitspersample/8, ext->header.numchannels, ext->fp);
                if(absr(v) > 1) fseek(org->fp, ((int) org->header.blockalign*floor(absr(v)-1)), SEEK_CUR);
            }
            else
            {
                fwrite(&w, ext->header.bitspersample/8, ext->header.numchannels, ext->fp);
            }
            if(aux > org->numsamples) break;
        }
        fclose(org->fp), fclose(ext->fp);
        remove("tempreverse.wav");
        return 1;
    }
}

int main()
{
    /*
        1. [v] Tocar um  udio
        2. [v] Recortar o trecho de um audio
        3. [?] Modificar o volume de um audio
        4. [v] Alterar a velocidade de um audio
        5. [x] Concatear um audio a outro
        6. [x] Separar os canais auditivos de um audio
        8. [x] Unir audios diferentes em cada canal
        9. [v] Alterar quantidade de bits
        10.[v] Alterar a quantidade de canais de um audio
    */

    char e;
    int i, j;
    float a, v;
    struct wavefmt aud, wav;
    struct wavefmt_length to, t;
    setlocale(LC_ALL, "Portuguese");

    do
    {
        printf("Olá, eu sou o manipulador de áudios \"Lambda\"!\n\t1. Tocar um áudio\n\t2. Recortar um intervalo\n\t3. Modificar o volume\n\t4. Modificar a velocidade\n\t5. Alterar quantidade de bits\n\t6. Alterar a quantidade de canais\n\t7. Exibir informações do arquivo.\nAperte 'ESC' para encerrar.\n");
        printf("Escolha: ");
        e = getch();
        if(isprint(e)) printf("%c", e);
        while((e < '1' || e > '7') && e != 27)
        {
            printf("\nEssa opção não existe. Digite novamente: ");
            fflush(stdin);
            e = getche();
            if(isprint(e)) printf("%c", e);
        }
        if(e != 27)
        {
            printf("\nTítulo do áudio: ");
            fflush(stdin);
            gets(aud.title);
            i = waveintro(&aud);
            while(i == 0)
            {
                printf("Nenhum áudio encontrado com esse título.\nDigite novamente: ");
                gets(aud.title);
                i = waveintro(&aud);
            }
            switch(e)
            {
            case '1':
                printf("\tTocando \"%s\"...\n", aud.filename);
                PlaySound(aud.filename, NULL, SND_SYNC);
                printf("\tTransmissão encerrada.\n");
                break;
            case '2':
                printf("Digite \"m:ss, m:ss\" do tempo inicial ao final:\n\t(Comprimento estimado do áudio: %d:%02d)\n", aud.length.min, aud.length.s);
                scanf("%d:%02d, %d:%02d", &to.min, &to.s, &t.min, &t.s);
                while(to.s >= 60 || t.s >= 60)
                {
                    printf("O período dos segundos é sempre menor que 60!\nDigite novamente: ");
                    scanf("%d:%02d, %d:%02d", &to.min, &to.s, &t.min, &t.s);
                }
                wavegetlength(&to, true);
                wavegetlength(&t, true);
                i = wavepatch(&wav, to.time, t.time, &aud);
                while(i <= 0)
                {
                    switch(i)
                    {
                    case 0:
                        printf("Não faz sentido haver(em) tempo(s) negativo(s)!");
                        break;
                    case -1:
                        printf("Não há como recortar um intervalo cujo início é maior que o final!");
                        break;
                    case -2:
                        printf("O(s) tempo(s) não pode(m) exceder o comprimento do áudio!");
                        break;
                    }
                    printf("\nDigite novamente: ");
                    scanf("%d:%02d, %d:%02d", &to.min, &to.s, &t.min, &t.s);
                    wavegetlength(&to, true);
                    wavegetlength(&t, true);
                    i = wavepatch(&wav, to.time, t.time, &aud);
                }
                printf("\tArquivo \"%s\" gerado com sucesso!\n", wav.filename);
                break;
            case '3':
                printf("Amplitude: ");
                scanf("%f", &a);
                i = wavemod(&wav, a, &aud);
                while(i <= 0)
                {
                    switch(i)
                    {
                    case 0:
                        printf("Não digite uma modulação negativa ou igual a zero!");
                        break;
                    case -1:
                        printf("O áudio não tem uma taxa de bits adequada (16).\n");
                        break;
                    case -2:
                        printf("A amplitude escolhida é alta demais para o arquivo lidar!");
                        break;
                    }
                    if(i == -1) break;
                    else
                    {
                        fflush(stdin);
                        printf("\nDigite novamente: ");
                        scanf("%f", &a);
                        i = wavemod(&wav, a, &aud);
                    }
                }
                if(i > 0) printf("\tArquivo \"%s\" gerado com sucesso!\n", wav.filename);
                break;
            case '4':
                printf("Velocidade: ");
                scanf("%f", &v);
                i = wavespeed(&wav, v, &aud);
                while(i == 0)
                {
                    printf("Não há como um arquivo ter velocidade de reprodução igual a 0.\nDigite novamente: ");
                    scanf("%f", &v);
                    i = wavespeed(&wav, v, &aud);
                }
                printf("\tArquivo \"%s\" gerado com sucesso!\n", wav.filename);
                break;
            case '5':
                printf("Digite a taxa de bits que voc  quer no arquivo final (8 ou 16).\nDigite a opção que seja igual ao do original para cancelar a operação.\n\t(Taxa de bits atual: %d)\n", aud.header.bitspersample);
                scanf("%d", &j);
                i = wavebps(&wav, j, &aud);
                while(i <= 0)
                {
                    switch(i)
                    {
                    case 0:
                        printf("Essa opção já é igual ao do arquivo original. Operação cancelada.\n");
                        break;
                    case 1:
                        printf("Essa opção não existe.");
                        break;
                    }
                    if(i == 0) break;
                    else
                    {
                        printf("\nDigite novamente: ");
                        scanf("%d", &j);
                        i = wavebps(&wav, j, &aud);
                    }
                }
                if(i > 0) printf("\tArquivo \"%s\" gerado com sucesso!\n", wav.filename);
                break;
            case '6':
                printf("Opções:\n1. Estéreo -> Mono\n2. Mono -> Estéreo\nDigite a opção que seja igual ao do original para cancelar a operação.\n\t(Quantidade de canais atual: %d)\n", aud.header.numchannels);
                scanf("%d", &j);
                i = wavechan(&wav, j, &aud);
                while(i <= 0)
                {
                    switch(i)
                    {
                    case 0:
                        printf("Essa opção já é igual ao do arquivo original. Operação cancelada.\n");
                        break;
                    case 1:
                        printf("Essa opção não existe.");
                        break;
                    }
                    if(i == 0) break;
                    else
                    {
                        printf("\nDigite novamente: ");
                        scanf("%d", &j);
                        i = wavechan(&wav, j, &aud);
                    }
                }
                if(i > 0) printf("\tArquivo \"%s\" gerado com sucesso!\n", wav.filename);
                break;
            case '7':
                printf("\n\tExibindo informações do arquivo:\n");
                wavelogprintf();
                break;
            }
            getch();
            system("cls");
        }
    } while (e != 27);

    printf("\n\nIteração encerrada.\n");
    return 0;
}
