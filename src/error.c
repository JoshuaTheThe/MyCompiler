
#include "lex/lexer.h"
#include <error.h>
#include <stdarg.h>

void comperror(token_stream_t *File, token_t ReferenceToken, const char *const fmt, ...)
{
        char ErrorBuffer[1024 * 2] = {0};
        char FormattedMessage[1024] = {0};
        va_list args;
        va_start(args, fmt);
        vsnprintf(FormattedMessage, sizeof(FormattedMessage), fmt, args);
        va_end(args);
        snprintf(ErrorBuffer, sizeof(ErrorBuffer), "%s:%ld:%ld: %s\n",
                File->Identifier, ReferenceToken.Line, ReferenceToken.Column,
                FormattedMessage);
        fprintf(stderr, "%s", ErrorBuffer);
        if (ReferenceToken.Line <= File->LineCount)
        {
                char Character = 0;
                fseek(File->fp, File->LineOffsets[ReferenceToken.Line - 1], SEEK_SET);
                while (Character != '\n' && Character != EOF)
                {
                        if (Character != 0)
                                fprintf(stderr, "%c", Character);
                        Character = fgetc(File->fp);
                }

                fprintf(stderr, "\n%*s^\n", (int)ReferenceToken.Column - 1, "");
        }

        exit(1);
}
