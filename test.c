#include <stdio.h>
#include <ctype.h>
#include <string.h>

void vigenere(char *text, char *key, int encrypt) {
    int key_len = strlen(key);
    int key_index = 0;
    char offset;

    for (int i = 0; text[i] != '\0'; i++) {
        if (isalpha(text[i])) {
            // Determine the offset for the current key character
            offset = isupper(key[key_index]) ? (key[key_index] - 'A') : (key[key_index] - 'a');

            if (isupper(text[i])) {
                if (encrypt) {
                    // Encrypt uppercase letters
                    text[i] = 'A' + (text[i] - 'A' + offset) % 26;
                } else {
                    // Decrypt uppercase letters
                    text[i] = 'A' + (text[i] - 'A' - offset + 26) % 26;
                }
            } else {
                if (encrypt) {
                    // Encrypt lowercase letters
                    text[i] = 'a' + (text[i] - 'a' + offset) % 26;
                } else {
                    // Decrypt lowercase letters
                    text[i] = 'a' + (text[i] - 'a' - offset + 26) % 26;
                }
            }

            // Move to the next key character and wrap around if necessary
            key_index = (key_index + 1) % key_len;
        }
    }
}

int main() {
    char text[1024];
    char key[256];
    int choice;

    printf("Enter text: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = '\0';  // Remove newline character

    printf("Enter key: ");
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = '\0';  // Remove newline character

    printf("Choose (1 for encrypt, 0 for decrypt): ");
    scanf("%d", &choice);

    vigenere(text, key, choice);

    printf("Output: %s\n", text);

    return 0;
}
