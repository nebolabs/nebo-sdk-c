#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nebo/nebo.h>

/*
 * Simple JSON parsing helpers for the calculator.
 * In a real app, use a JSON library (cJSON, jansson, etc.).
 */
static double json_get_number(const char *json, const char *key) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ') p++;
    return atof(p);
}

static int json_get_string(const char *json, const char *key, char *out, int maxlen) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return -1;
    p += strlen(pattern);
    int i = 0;
    while (*p && *p != '"' && i < maxlen - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 0;
}

static int calculator_execute(const char *input_json, char **output, int *is_error) {
    char action[32] = {0};
    if (json_get_string(input_json, "action", action, sizeof(action)) != 0) {
        *output = strdup("Missing action");
        *is_error = 1;
        return 0;
    }

    double a = json_get_number(input_json, "a");
    double b = json_get_number(input_json, "b");
    double result = 0;

    if (strcmp(action, "add") == 0) {
        result = a + b;
    } else if (strcmp(action, "subtract") == 0) {
        result = a - b;
    } else if (strcmp(action, "multiply") == 0) {
        result = a * b;
    } else if (strcmp(action, "divide") == 0) {
        if (b == 0) {
            *output = strdup("Division by zero");
            *is_error = 1;
            return 0;
        }
        result = a / b;
    } else {
        char buf[128];
        snprintf(buf, sizeof(buf), "Unknown action: %s", action);
        *output = strdup(buf);
        *is_error = 1;
        return 0;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "%g %s %g = %g", a, action, b, result);
    *output = strdup(buf);
    *is_error = 0;
    return 0;
}

int main(void) {
    /* Build the schema using the SDK builder */
    const char *actions[] = {"add", "subtract", "multiply", "divide", NULL};
    nebo_schema_builder_t *sb = nebo_schema_new(actions);
    nebo_schema_number(sb, "a", "First operand", 1);
    nebo_schema_number(sb, "b", "Second operand", 1);
    char *schema = nebo_schema_build(sb);
    nebo_schema_free(sb);

    nebo_tool_handler_t calculator = {
        .name = "calculator",
        .description = "Performs arithmetic calculations.",
        .schema = schema,
        .execute = calculator_execute,
        .requires_approval = NULL,
    };

    nebo_app_t *app = nebo_app_new();
    nebo_app_register_tool(app, &calculator);
    int ret = nebo_app_run(app);

    free(schema);
    return ret;
}
