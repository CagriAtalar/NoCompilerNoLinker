#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h> // system(), malloc(), free() için
#include <elf.h>
#include <sys/stat.h> // chmod() için

// Makine kodunun yükleneceği sanal adresin başlangıcı
#define VADDR_START 0x400000

int main(int argc, char *argv[]) {
    // --- 1. Argümanları Kontrol Et ---
    if (argc != 2) {
        fprintf(stderr, "Kullanim: %s <assembly_dosyasi.asm>\n", argv[0]);
        return 1;
    }

    // --- 2. Dosya İsimlerini Hazırla ---
    char *asm_file = argv[1];
    char *output_file = strdup(asm_file);
    char *bin_file = strdup(asm_file);

    // .asm uzantısını dosya sonundan silerek çıktı ve .bin isimlerini oluştur
    char *dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';

    // .bin uzantısını ekle
    strcpy(bin_file, output_file);
    strcat(bin_file, ".bin");

    printf("-> Assembly Dosyasi: %s\n", asm_file);
    printf("-> Gecici Binary Dosyasi: %s\n", bin_file);
    printf("-> Cikti Dosyasi: %s\n", output_file);

    // --- 3. NASM ile Assembly Kodunu Derle ---
    char command[256];
    snprintf(command, sizeof(command), "nasm -f bin \"%s\" -o \"%s\"", asm_file, bin_file);

    printf("-> NASM komutu calistiriliyor: %s\n", command);
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "HATA: NASM derlemesi basarisiz oldu. '%s' dosyasinda hata olabilir mi?\n", asm_file);
        free(output_file);
        free(bin_file);
        return 1;
    }

    // --- 4. Derlenmiş Makine Kodunu Oku ---
    FILE *f_bin = fopen(bin_file, "rb");
    if (!f_bin) {
        perror("HATA: Gecici binary dosyasi acilamadi");
        free(output_file);
        free(bin_file);
        return 1;
    }

    fseek(f_bin, 0, SEEK_END);
    long code_size = ftell(f_bin);
    fseek(f_bin, 0, SEEK_SET);

    unsigned char *code = (unsigned char *)malloc(code_size);
    if (!code) {
        fprintf(stderr, "HATA: Makine kodu icin bellek ayrılamadı.\n");
        fclose(f_bin);
        free(output_file);
        free(bin_file);
        return 1;
    }

    if (fread(code, 1, code_size, f_bin) != code_size) {
        fprintf(stderr, "HATA: Makine kodu okunamadi.\n");
        fclose(f_bin);
        free(code);
        free(output_file);
        free(bin_file);
        return 1;
    }
    fclose(f_bin);
    printf("-> Makine kodu başarıyla okundu (%ld bayt).\n", code_size);

    // --- 5. ELF Başlıklarını Ayarla ---
    Elf64_Ehdr eh = {0};
    Elf64_Phdr ph = {0};

    // Kodun dosya içindeki başlangıç ofseti (header'lardan hemen sonra)
    uint64_t code_offset = sizeof(eh) + sizeof(ph);

    // ELF Header
    memcpy(eh.e_ident, ELFMAG, SELFMAG);
    eh.e_ident[EI_CLASS]   = ELFCLASS64;
    eh.e_ident[EI_DATA]    = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_ident[EI_OSABI]   = ELFOSABI_SYSV;
    eh.e_type              = ET_EXEC;
    eh.e_machine           = EM_X86_64;
    eh.e_version           = EV_CURRENT;
    eh.e_entry             = VADDR_START + code_offset; // Giriş noktası: Sanal adres + kod ofseti
    eh.e_phoff             = sizeof(Elf64_Ehdr);
    eh.e_ehsize            = sizeof(Elf64_Ehdr);
    eh.e_phentsize         = sizeof(Elf64_Phdr);
    eh.e_phnum             = 1;

    // Program Header
    ph.p_type   = PT_LOAD;
    ph.p_offset = 0;
    ph.p_vaddr  = VADDR_START;
    ph.p_paddr  = VADDR_START;
    ph.p_filesz = code_offset + code_size; // Dosyadaki boyutu: Header'lar + kod
    ph.p_memsz  = code_offset + code_size; // Bellekteki boyutu: Header'lar + kod
    ph.p_flags  = PF_R | PF_X; // Okuma ve Çalıştırma izni
    ph.p_align  = 0x1000;

    // --- 6. Çalıştırılabilir ELF Dosyasını Yaz ---
    FILE *f_out = fopen(output_file, "wb");
    if (!f_out) {
        perror("HATA: Cikti dosyasi olusturulamadi");
        free(code);
        free(output_file);
        free(bin_file);
        return 1;
    }

    fwrite(&eh, 1, sizeof(eh), f_out);
    fwrite(&ph, 1, sizeof(ph), f_out);
    fwrite(code, 1, code_size, f_out);

    fclose(f_out);
    printf("-> ELF dosyasi '%s' basariyla olusturuldu.\n", output_file);

    // --- 7. Çıktı Dosyasını Çalıştırılabilir Yap ---
    if (chmod(output_file, 0755) != 0) {
        perror("UYARI: Dosyaya calistirma izni verilemedi (chmod +x)");
    }

    // --- 8. Temizlik ---
    remove(bin_file); // Geçici .bin dosyasını sil
    free(code);
    free(output_file);
    free(bin_file);

    printf("-> Islem tamamlandi.\n");
    return 0;
}
