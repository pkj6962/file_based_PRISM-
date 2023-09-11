#include "mpitracer.h"
#include <cstring>

namespace danzer
{

#define FASTCDC_CLAMP(x, a, b) ((x < (a)) ? (a) : ((x > b) ? b : x))
#define FASTCDC_DIVCEIL(a, b) ((a) / (b) + ((a) % (b) ? 1 : 0))
#define FASTCDC_MASK(x) (uint32_t)((1 << FASTCDC_CLAMP(x, 1, 31)) - 1)

#define AVERAGE_MIN (1 << 8)
#define AVERAGE_MAX (1 << 28)
#define MINIMUM_MIN (AVERAGE_MIN >> 2)
#define MINIMUM_MAX (AVERAGE_MAX >> 2)
#define MAXIMUM_MIN (AVERAGE_MIN << 2)
#define MAXIMUM_MAX (AVERAGE_MAX << 2)

    static const uint32_t GEAR[] = {
        0x5C95C078, 0x22408989, 0x2D48A214, 0x12842087, 0x530F8AFB, 0x474536B9,
        0x2963B4F1, 0x44CB738B, 0x4EA7403D, 0x4D606B6E, 0x074EC5D3, 0x3AF39D18,
        0x726003CA, 0x37A62A74, 0x51A2F58E, 0x7506358E, 0x5D4AB128, 0x4D4AE17B,
        0x41E85924, 0x470C36F7, 0x4741CBE1, 0x01BB7F30, 0x617C1DE3, 0x2B0C3A1F,
        0x50C48F73, 0x21A82D37, 0x6095ACE0, 0x419167A0, 0x3CAF49B0, 0x40CEA62D,
        0x66BC1C66, 0x545E1DAD, 0x2BFA77CD, 0x6E85DA24, 0x5FB0BDC5, 0x652CFC29,
        0x3A0AE1AB, 0x2837E0F3, 0x6387B70E, 0x13176012, 0x4362C2BB, 0x66D8F4B1,
        0x37FCE834, 0x2C9CD386, 0x21144296, 0x627268A8, 0x650DF537, 0x2805D579,
        0x3B21EBBD, 0x7357ED34, 0x3F58B583, 0x7150DDCA, 0x7362225E, 0x620A6070,
        0x2C5EF529, 0x7B522466, 0x768B78C0, 0x4B54E51E, 0x75FA07E5, 0x06A35FC6,
        0x30B71024, 0x1C8626E1, 0x296AD578, 0x28D7BE2E, 0x1490A05A, 0x7CEE43BD,
        0x698B56E3, 0x09DC0126, 0x4ED6DF6E, 0x02C1BFC7, 0x2A59AD53, 0x29C0E434,
        0x7D6C5278, 0x507940A7, 0x5EF6BA93, 0x68B6AF1E, 0x46537276, 0x611BC766,
        0x155C587D, 0x301BA847, 0x2CC9DDA7, 0x0A438E2C, 0x0A69D514, 0x744C72D3,
        0x4F326B9B, 0x7EF34286, 0x4A0EF8A7, 0x6AE06EBE, 0x669C5372, 0x12402DCB,
        0x5FEAE99D, 0x76C7F4A7, 0x6ABDB79C, 0x0DFAA038, 0x20E2282C, 0x730ED48B,
        0x069DAC2F, 0x168ECF3E, 0x2610E61F, 0x2C512C8E, 0x15FB8C06, 0x5E62BC76,
        0x69555135, 0x0ADB864C, 0x4268F914, 0x349AB3AA, 0x20EDFDB2, 0x51727981,
        0x37B4B3D8, 0x5DD17522, 0x6B2CBFE4, 0x5C47CF9F, 0x30FA1CCD, 0x23DEDB56,
        0x13D1F50A, 0x64EDDEE7, 0x0820B0F7, 0x46E07308, 0x1E2D1DFD, 0x17B06C32,
        0x250036D8, 0x284DBF34, 0x68292EE0, 0x362EC87C, 0x087CB1EB, 0x76B46720,
        0x104130DB, 0x71966387, 0x482DC43F, 0x2388EF25, 0x524144E1, 0x44BD834E,
        0x448E7DA3, 0x3FA6EAF9, 0x3CDA215C, 0x3A500CF3, 0x395CB432, 0x5195129F,
        0x43945F87, 0x51862CA4, 0x56EA8FF1, 0x201034DC, 0x4D328FF5, 0x7D73A909,
        0x6234D379, 0x64CFBF9C, 0x36F6589A, 0x0A2CE98A, 0x5FE4D971, 0x03BC15C5,
        0x44021D33, 0x16C1932B, 0x37503614, 0x1ACAF69D, 0x3F03B779, 0x49E61A03,
        0x1F52D7EA, 0x1C6DDD5C, 0x062218CE, 0x07E7A11A, 0x1905757A, 0x7CE00A53,
        0x49F44F29, 0x4BCC70B5, 0x39FEEA55, 0x5242CEE8, 0x3CE56B85, 0x00B81672,
        0x46BEECCC, 0x3CA0AD56, 0x2396CEE8, 0x78547F40, 0x6B08089B, 0x66A56751,
        0x781E7E46, 0x1E2CF856, 0x3BC13591, 0x494A4202, 0x520494D7, 0x2D87459A,
        0x757555B6, 0x42284CC1, 0x1F478507, 0x75C95DFF, 0x35FF8DD7, 0x4E4757ED,
        0x2E11F88C, 0x5E1B5048, 0x420E6699, 0x226B0695, 0x4D1679B4, 0x5A22646F,
        0x161D1131, 0x125C68D9, 0x1313E32E, 0x4AA85724, 0x21DC7EC1, 0x4FFA29FE,
        0x72968382, 0x1CA8EEF3, 0x3F3B1C28, 0x39C2FB6C, 0x6D76493F, 0x7A22A62E,
        0x789B1C2A, 0x16E0CB53, 0x7DECEEEB, 0x0DC7E1C6, 0x5C75BF3D, 0x52218333,
        0x106DE4D6, 0x7DC64422, 0x65590FF4, 0x2C02EC30, 0x64A9AC67, 0x59CAB2E9,
        0x4A21D2F3, 0x0F616E57, 0x23B54EE8, 0x02730AAA, 0x2F3C634D, 0x7117FC6C,
        0x01AC6F05, 0x5A9ED20C, 0x158C4E2A, 0x42B699F0, 0x0C7C14B3, 0x02BD9641,
        0x15AD56FC, 0x1C722F60, 0x7DA1AF91, 0x23E0DBCB, 0x0E93E12B, 0x64B2791D,
        0x440D2476, 0x588EA8DD, 0x4665A658, 0x7446C418, 0x1877A774, 0x5626407E,
        0x7F63BD46, 0x32D2DBD8, 0x3C790F4A, 0x772B7239, 0x6F8B2826, 0x677FF609,
        0x0DC82C11, 0x23FFE354, 0x2EAC53A6, 0x16139E09, 0x0AFD0DBC, 0x2A4D4237,
        0x56A368C7, 0x234325E4, 0x2DCE9187, 0x32E8EA7E};

    uint32_t normal_size(const uint32_t mi, const uint32_t av, const uint32_t len)
    {
        uint32_t off = mi + FASTCDC_DIVCEIL(mi, 2);
        if (off > av)
            off = av;
        uint32_t sz = av - off;
        if (sz > len)
            return len;
        return sz;
    }

    uint32_t cut(const uint8_t *src, const uint32_t len, const uint32_t mi, const uint32_t ma, const uint32_t ns, const uint32_t mask_s, const uint32_t mask_l)
    {
        uint32_t n, fp = 0, i = (len < mi) ? len : mi;
        n = (ns < len) ? ns : len;
        for (; i < n; i++)
        {
            fp = (fp >> 1) + GEAR[src[i]];
            //*tf_name << fp <<"\n";
            if (!(fp & mask_s))
                return i + 1;
        }
        n = (ma < len) ? ma : len;
        for (; i < n; i++)
        {
            fp = (fp >> 1) + GEAR[src[i]];
            //*tf_name << fp << "\n";
            if (!(fp & mask_l))
                return i + 1;
        }
        return i;
    }

    fcdc_ctx Dedupe::fastcdc_init(uint32_t mi, uint32_t av, uint32_t ma)
    {
        uint32_t bits = (uint32_t)round(log2(av));
        fcdc_ctx ctx = {
            .mi = FASTCDC_CLAMP(mi, MINIMUM_MIN, MINIMUM_MAX),
            .av = FASTCDC_CLAMP(av, AVERAGE_MIN, AVERAGE_MAX),
            .ma = FASTCDC_CLAMP(ma, MAXIMUM_MIN, MAXIMUM_MAX),
            .ns = normal_size(mi, av, ma),
            .mask_s = FASTCDC_MASK(bits + 1),
            .mask_l = FASTCDC_MASK(bits - 1),
            .pos = 0};
        return ctx;
    }

    size_t Dedupe::fastcdc_update(fcdc_ctx *ctx, uint8_t *data, size_t len, int end, chunk_vec *cv)
    {
        size_t offset = 0;
        while (((len - offset) >= ctx->ma) || (end && (offset < len)))
        {
            uint32_t cp = cut(data + offset, len - offset, ctx->mi, ctx->ma, ctx->ns,
                              ctx->mask_s, ctx->mask_l); // Returns the chunk length
            chunk blk = {.offset = ctx->pos + offset, .len = cp};
            kv_push(chunk, *cv, blk);
            offset += cp;
        }
        ctx->pos += offset;
        return offset;
    }

    size_t Dedupe::fastcdc_stream(FILE *stream, uint32_t mi, uint32_t av, uint32_t ma, chunk_vec *cv)
    {
        size_t offset = 0;
        int end = 0;
        fcdc_ctx cdc = fastcdc_init(mi, av, ma), *ctx = &cdc;
        size_t rs = ctx->ma * 4;
        rs = FASTCDC_CLAMP(rs, 0, UINT32_MAX);
        // printf("rs: %ld\n", rs);
        uint8_t *data = (uint8_t *)malloc(rs);
        while (!end)
        {
            size_t ar = fread(data, 1, rs, stream);
            end = feof(stream);
            offset += fastcdc_update(ctx, data, ar, end, cv);
            fseek(stream, offset, SEEK_SET);
        }
        free(data);
        return kv_size(*cv);
    }

    /*
        -f option 1:
        file size percentile
    */
    void FStat::measure_file_sizes(string directory_path, ofstream &output_file)
    {
        cout << "Measuring file sizes session start . . ." << endl;

        long long dir_capacity = 0;
        cout << "[1] Iterate through whole directory . . ." << endl;
        // (1) Iterate through whole directory.
        for (auto const &dir_entry : filesystem::recursive_directory_iterator(directory_path))
        {
            if (filesystem::is_symlink(dir_entry))
            {
                cout << "Symlink encountered !" << endl;
                continue;
            }

            if (dir_entry.is_regular_file())
            {
                string fname = filesystem::absolute(dir_entry.path().string());

                // cout << "File name: " << fname << endl;
                // cout << "File size: " << filesystem::file_size(fname) << endl;

                fsize_table.push_back({filesystem::file_size(fname), fname});

                dir_capacity += filesystem::file_size(fname);
            }
        }
        cout << "dir_capacity == " << dir_capacity << endl;
        output_file << "dir_capacity == " << dir_capacity << endl
                    << endl;

        cout << endl;
        cout << "[2] Analyze the file size table information . . ." << endl;

        cout << "-- length: " << fsize_table.size() << endl;
        output_file << "-- length: " << fsize_table.size() << endl;

        // (2) Sort the list.
        sort(fsize_table.begin(), fsize_table.end());
        // for (auto x : fsize_table)
        //     cout << x.first << "               " << x.second << endl;
        // cout << endl;

        // (3) Calculate specific percentile file count values.
        // In order to avoid numerical overflow, use 8 bytes integer type!
        long long percentile_10 = fsize_table.size() * 0.1;  // cout << percentile_10 << endl;
        long long percentile_25 = fsize_table.size() * 0.25; // cout << percentile_25 << endl;
        long long percentile_50 = fsize_table.size() * 0.5;  // cout << percentile_50 << endl;
        long long percentile_75 = fsize_table.size() * 0.75; // cout << percentile_75 << endl;
        long long percentile_90 = fsize_table.size() * 0.9;  // cout << percentile_90 << endl;

        long long fs_10 = 0;
        long long fs_25 = 0;
        long long fs_50 = 0;
        long long fs_75 = 0;
        long long fs_90 = 0;

        long long count = 0;

        // (4) Iterate the list one more time - check the corresponding file sizes for each file count percentile.
        for (auto x : fsize_table)
        {
            // cout << count << "      " << x.first << "            " << x.second << endl;
            // output_file << x.second << ", " << x.first << endl;

            total_file_count++;
            total_file_size += x.first;

            if (count == percentile_10)
            {
                cout << "10%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "10%, " << count << ", " << x.first << ", " << x.second << endl;
                fs_10 = x.first;
            }
            else if (count == percentile_25)
            {
                cout << "25%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "25%, " << count << ", " << x.first << ", " << x.second << endl;
                fs_25 = x.first;
            }
            else if (count == percentile_50)
            {
                cout << "50%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "50%, " << count << ", " << x.first << ", " << x.second << endl;
                fs_50 = x.first;
            }
            else if (count == percentile_75)
            {
                cout << "75%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "75%, " << count << ", " << x.first << ", " << x.second << endl;
                fs_75 = x.first;
            }
            else if (count == percentile_90)
            {
                cout << "90%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "90%, " << count << ", " << x.first << ", " << x.second << endl;
                fs_90 = x.first;
            }

            count++;
        }

        cout << "\n";
        cout << "[3] Calculate the aggregate information . . ." << endl;
        // (5) Additionally, let's calcualte the mean file size as well.
        mean_file_size = (long long)((double)total_file_size / (double)total_file_count);

        cout << "total_file_count : " << total_file_count << endl;
        cout << "total_file_size  : " << total_file_size << endl;
        cout << "mean_file_size   : " << mean_file_size << endl;

        output_file << endl;
        output_file << "total_file_count, " << total_file_count << endl;
        output_file << "total_file_size, " << total_file_size << endl;
        output_file << "mean_file_size, " << mean_file_size << endl;

        // (6) csv format
        cout << endl;
        cout << total_file_count << ", " << total_file_size << ", " << mean_file_size << ", " << fs_10 << ", " << fs_25 << ", " << fs_50 << ", " << fs_75 << ", " << fs_90 << endl;

        output_file << endl;
        output_file << total_file_count << ", " << total_file_size << ", " << mean_file_size << ", " << fs_10 << ", " << fs_25 << ", " << fs_50 << ", " << fs_75 << ", " << fs_90 << endl;

        return;
    }

    /*
        -f option 2:
        cumulative file size percentile
    */
    void FStat::measure_cumulative_fs(string directory_path, ofstream &output_file)
    {
        cout << "* Cumulative distribution session start . . ." << endl;

        long long dir_capacity = 0;
        cout << "[1] Iterate through whole directory . . ." << endl;
        // (1) Iterate through whole directory.
        for (auto const &dir_entry : filesystem::recursive_directory_iterator(directory_path))
        {
            if (filesystem::is_symlink(dir_entry))
            {
                cout << "Symlink encountered !" << endl;
                continue;
            }

            if (dir_entry.is_regular_file())
            {
                string fname = filesystem::absolute(dir_entry.path().string());

                // cout << "File name: " << fname << endl;
                // cout << "File size: " << filesystem::file_size(fname) << endl;

                fsize_table.push_back({filesystem::file_size(fname), fname});

                dir_capacity += filesystem::file_size(fname);
            }
        }
        cout << "dir_capacity == " << dir_capacity << endl;
        output_file << "dir_capacity == " << dir_capacity << endl
                    << endl;

        cout << endl;
        cout << "[2] Analyze the file size table information . . ." << endl;

        cout << "-- length: " << fsize_table.size() << endl;
        output_file << "-- length: " << fsize_table.size() << endl;

        // (2) Sort the list.
        sort(fsize_table.begin(), fsize_table.end());
        // for (auto x : fsize_table)
        //     cout << x.first << "               " << x.second << endl;
        // cout << endl;

        // (3) Calculate specific percentile file count values.
        // In order to avoid numerical overflow, use 8 bytes integer type!
        long long c_percentile_10 = dir_capacity * 0.1;  // cout << percentile_10 << endl;
        long long c_percentile_25 = dir_capacity * 0.25; // cout << percentile_25 << endl;
        long long c_percentile_50 = dir_capacity * 0.5;  // cout << percentile_50 << endl;
        long long c_percentile_75 = dir_capacity * 0.75; // cout << percentile_75 << endl;
        long long c_percentile_90 = dir_capacity * 0.9;  // cout << percentile_90 << endl;

        long long c_fs_10 = 0;
        long long c_fs_25 = 0;
        long long c_fs_50 = 0;
        long long c_fs_75 = 0;
        long long c_fs_90 = 0;

        long long count = 0;

        long long sum_capacity = 0;
        // (4) Iterate the list one more time - check the corresponding file sizes for each file count percentile.
        for (auto x : fsize_table)
        {
            // cout << count << "      " << x.first << "            " << x.second << endl;
            // output_file << x.second << ", " << x.first << endl;

            total_file_count++;
            total_file_size += x.first;

            if (c_fs_10 == 0 && total_file_size >= c_percentile_10)
            {
                cout << "10%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "10%, " << count << ", " << x.first << ", " << x.second << endl;
                c_fs_10 = x.first; // assign
            }
            else if (c_fs_25 == 0 && total_file_size >= c_percentile_25)
            {
                cout << "25%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "25%, " << count << ", " << x.first << ", " << x.second << endl;
                c_fs_25 = x.first; // assign
            }
            else if (c_fs_50 == 0 && total_file_size >= c_percentile_50)
            {
                cout << "50%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "50%, " << count << ", " << x.first << ", " << x.second << endl;
                c_fs_50 = x.first; // assign
            }
            else if (c_fs_75 == 0 && total_file_size >= c_percentile_75)
            {
                cout << "75%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "75%, " << count << ", " << x.first << ", " << x.second << endl;
                c_fs_75 = x.first; // assign
            }
            else if (c_fs_90 == 0 && total_file_size >= c_percentile_90)
            {
                cout << "90%    " << count << "      " << x.first << "            " << x.second << endl;
                output_file << "90%, " << count << ", " << x.first << ", " << x.second << endl;
                c_fs_90 = x.first;
            }

            count++;
        }

        cout << "\n";
        cout << "[3] Calculate the aggregate information . . ." << endl;
        // (5) Additionally, let's calcualte the mean file size as well.
        mean_file_size = (long long)((double)total_file_size / (double)total_file_count);

        cout << "total_file_count : " << total_file_count << endl;
        cout << "total_file_size  : " << total_file_size << endl;
        cout << "mean_file_size   : " << mean_file_size << endl;

        output_file << endl;
        output_file << "total_file_count, " << total_file_count << endl;
        output_file << "total_file_size, " << total_file_size << endl;
        output_file << "mean_file_size, " << mean_file_size << endl;

        // (6) csv format
        cout << endl;
        cout << total_file_count << ", " << total_file_size << ", " << mean_file_size << ", " << c_fs_10 << ", " << c_fs_25 << ", " << c_fs_50 << ", " << c_fs_75 << ", " << c_fs_90 << endl;

        output_file << endl;
        output_file << total_file_count << ", " << total_file_size << ", " << mean_file_size << ", " << c_fs_10 << ", " << c_fs_25 << ", " << c_fs_50 << ", " << c_fs_75 << ", " << c_fs_90 << endl;

        return;
    }

    /*
        -f option 3:
        file extension
    */
    void FStat::measure_file_extensions(string directory_path, ofstream &output_file)
    {
        cout << "** Measuring file extensions session start . . ." << endl;

        // (1) Iterate through whole directory.
        cout << "[1] Iterate through whole directory . . ." << endl;
        for (auto const &dir_entry : filesystem::recursive_directory_iterator(directory_path))
        {
            if (filesystem::is_symlink(dir_entry))
            {
                cout << "Symlink encountered" << endl;
                continue;
            }

            if (dir_entry.is_regular_file())
            {
                string fname = filesystem::absolute(dir_entry.path().string());
                size_t end = fname.length();
                size_t cur = fname.rfind('.');
                string file_extension;

                if (cur == string::npos)
                    file_extension = "none"; // no file extension
                else
                    file_extension = fname.substr(cur + 1, end);

                cout << "File extension: " << file_extension << endl;
                output_file << "File Extension: " << file_extension << endl;

                // (2) Make hash table for file extension.
                fe_table[file_extension]++;
            }
        }

        for (auto x : fe_table)
            cout << x.first << "        " << x.second << endl;

        return;
    }

    void Dedupe::chunk_full_file(string file_name, ofstream &trace_file)
    {
        cout << "Full file chunk: " << file_name << endl;
        // Consider file as a single chunk
        unsigned char temp_fp[SHA_DIGEST_LENGTH];
        string buffer = readFile(file_name);
        int f_size = buffer.length();

        SHA1((const unsigned char *)buffer.c_str(), f_size, temp_fp);

        std::string fp = GetHexRepresentation(temp_fp, SHA_DIGEST_LENGTH);

        trace_file << "\nFingerprint: " + fp;
    }

    void Dedupe::chunk_cdc(string buffer, ofstream &trace_file)
    {
        unsigned char temp_fp[SHA_DIGEST_LENGTH];
        const char *chunk = buffer.c_str();
        int tmp_chunk_size = buffer.length();
        SHA1((const unsigned char *)chunk, tmp_chunk_size, temp_fp);
        string fp = GetHexRepresentation(temp_fp, SHA_DIGEST_LENGTH);
        trace_file << fp + "\n";
    }

    void Dedupe::chunk_cdc(string buffer, ofstream &trace_file, chunk_vec *cv, uint64_t *kv_count)
    {
        // parition the file based on window size of cdc
        // first read the file and then pass the buffer
        if (buffer.empty())
        {
            return;
        }
        uint64_t f_size = buffer.length();
        cerr << "Buffer length: " << f_size << endl;
        uint64_t f_pos = 0;
        uint64_t file_size = f_size;
        uint64_t chunk_cnt = *kv_count;
        cerr << "chunk_cnt: " << chunk_cnt << endl;
        int tmp_chunk_size = 0;
        while (f_pos < file_size)
        {
            // cerr<<"f_pos: "<<f_pos<<endl;
            const char *chunk;
            if (f_pos > file_size)
            {
                break;
            }

            tmp_chunk_size = kv_A(*cv, chunk_cnt).len;
            if (chunk_cnt > kv_size(*cv))
            {
                cerr << "Chunk count increased the cv size. chunk_cnt: " << chunk_cnt << endl;
            }
            if ((f_pos + tmp_chunk_size) > file_size)
            {
                string tmp_chunk = buffer.substr(f_pos);
                if (tmp_chunk_size > tmp_chunk.length())
                {
                    chunk = string(tmp_chunk_size - tmp_chunk.length(), '0').c_str(); // Fill constructor of string class
                }
                else
                {
                    chunk = tmp_chunk.c_str();
                }
            }
            else
            {
                chunk = buffer.substr(f_pos, tmp_chunk_size).c_str();
                // cerr<<"Else - tmp_chunk_size: "<<tmp_chunk_size<<"\n";
            }
            f_pos += tmp_chunk_size;
            f_size -= tmp_chunk_size;
            chunk_cnt += 1;
            unsigned char temp_fp[SHA_DIGEST_LENGTH];
            SHA1((const unsigned char *)chunk, tmp_chunk_size, temp_fp);
            string fp = GetHexRepresentation(temp_fp, SHA_DIGEST_LENGTH);
            trace_file << fp + "\n";
        }
        *kv_count = chunk_cnt;
    }

    string Dedupe::GetHexRepresentation(const unsigned char *Bytes, size_t Length)
    {
        std::string ret;
        ret.reserve(Length * 2);
        for (const unsigned char *ptr = Bytes; ptr < Bytes + Length; ++ptr)
        {
            char buf[3];
            sprintf(buf, "%02x", (*ptr) & 0xff);
            ret += buf;
        }
        return ret;
    }

    string Dedupe::readFile(const string &fileName)
    {
        ifstream ifs(fileName.c_str(), ios::binary);
        if (!ifs)
        {
            cout << "File openning failed\n";
            exit(0);
        }

        streampos ifs_start, ifs_end;
        ifs_start = ifs.tellg();
        ifs.seekg(0, ios::end);
        ifs_end = ifs.tellg();
        ifstream::pos_type fileSize = ifs_end - ifs_start;
        //*tf_name << "File name: "+fileName;
        ifs.seekg(0, ios::beg);

        vector<char> bytes(fileSize);
        ifs.read(&bytes[0], fileSize);

        ifs.close();
        return string(&bytes[0], fileSize);
    }

    void Dedupe::enqueue(OST_queue *ost_q, object_task task) {
        pthread_mutex_lock(&ost_q->mutex);
        // Enqueue task
        ost_q->taskQueue.push(task);
        pthread_mutex_unlock(&ost_q->mutex);
    }

    object_task Dedupe::dequeue(OST_queue *ost_q) {
        pthread_mutex_lock(&ost_q->mutex);

        while (ost_q->taskQueue.empty()) {
            pthread_cond_wait(&ost_q->cond, &ost_q->mutex);
        }
        // Dequeue task
        object_task task = ost_q->taskQueue.front();
        ost_q->taskQueue.pop();

        pthread_cond_signal(&ost_q->cond);
        pthread_mutex_unlock(&ost_q->mutex);

        return task;
    }

    void* Dedupe::readerThreadStarter(void* arg) {
    	Dedupe* self = static_cast<Dedupe*>(arg);
    	return self->readerThread();
    }

    void* Dedupe::commThreadStarter(void* arg) {
    	Dedupe* self = static_cast<Dedupe*>(arg);
    	return self->commThread();
    }
     void* Dedupe::workerThreadStarter(void* arg) {
        ThreadArgs* args = static_cast<ThreadArgs*>(arg);
        return args->instance->workerThread(args->index);
    }
    
    void* Dedupe::commThread() {
        int msg_cnt, task_cmt;
	int total_cnt = 0;
        MPI_Status status;

        //test code 
        static int file_cnt = 0; 

		char * buffer = (char*)malloc(sizeof(MAX_FILE_PATH_LEN) * 2000);
		string *file_list = new string[2000]; 
    	//char * buffer = (char*)malloc(sizeof(object_task) * TASK_QUEUE_FULL);
        if (buffer == NULL)
            cout << "Memory allocation error on commThread\n";
        while(1) {
			int flag, msg_size, rc; 

			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            task_cnt = status.MPI_TAG;

            // test code
            file_cnt += task_cnt; 

            MPI_Get_count(&status, MPI_CHAR, &msg_size); 

			if(task_cnt)
            {
                rc = MPI_Recv(buffer, msg_size, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
		
        	}
            else
			{
				rc = MPI_Recv(buffer, msg_size, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
	
			}	    
			if (rc != MPI_SUCCESS)
			{
				cout << "receive error: " << errno << this->rank << endl; 
			}
            if (strncmp(buffer, TERMINATION_MSG, strlen(TERMINATION_MSG)) == 0) {
                shutdown_flag = true;
				cout << "comm terminated " << rank << endl;
                printf("rank\t%d\tfile_cnt\t%d\n", rank, file_cnt);
                printf("rank\t%d\tqueu_siz\t%d\n", rank, file_queue.queue.size());
                


				break;
            }
            
            
            file_tasks_decomposition(buffer, task_cnt); 

            //MPI_Get_count(&status, MPI_CHAR, &msg_cnt);  
            //object_tasks_decomposition(buffer, task_num);
//			this->task_cnt_per_rank += task_num; 
		}
        
		free(buffer);
        return NULL;
    }

    void Dedupe::file_tasks_decomposition(const char* Msg, int task_cnt)
    {
        string * file_task = new string[task_cnt]; 
        
        string message(Msg); 
        message += '\n'; 
        istringstream iss(message); 

        for (int i = 0; i < task_cnt; i++)
        {
            getline(iss, file_task[i]); 	  
        }	    
        file_task_enqueue_batch(file_task, task_cnt);
    }

    void Dedupe::file_task_enqueue_batch(string file_task[], int task_cnt)
    {

        // TODO: Initilaization code에 mutex및 cond 초기화해주기
        pthread_mutex_lock(&file_queue.mutex); 
        for (int i = 0; i < task_cnt; i++)
        {
           file_queue.queue.push(file_task[i]);    
        }
        pthread_mutex_unlock(&file_queue.mutex); 
    }




//     void Dedupe::object_tasks_decomposition(const char* Msg, int cnt){
//         object_task * task; 
//         const char * p = Msg; 
//         for (int i = 0; i < cnt; i++){
//             task = object_task_deserialization(p);
// 	    if(task == NULL){
// 		cout << "Deserialization failed\n";
// 	    }
//             p += sizeof(object_task); 
            
//             // Check if this ost have been seen
//             auto it = ost_map.find(task->ost);
//             if (it == ost_map.end()) {
//                 // If not, create new queue
// //               this->ost_q.push_back(OST_queue());
// 		        cout << "new ost queue\n";
//                 // and add a mapping
//                 ost_map[task->ost] = this->ost_cnt++;
//             }

//             int local_ost_idx = ost_map[task->ost];
// 	    if(local_ost_idx >= ost_q.size())
// 			cout <<"invalid index\n";
//             enqueue(&(this->ost_q[local_ost_idx]), *task);
//         }
// 	return;
//     }

    void* Dedupe::readerThread(){
        Buffer * buffer;
        string file_path; 
        struct stat64 sb; 
        uint64_t file_size, bytesRead; 
        int fd; 

        // test code 
        static int test_open_cnt = 0;


        while(1)
        {
            bool taskFound = false; 
            while(!file_queue.queue.empty())
            {
                // TestCode
                cout << "rank " << rank << ' ' << "a1\n"; 

                taskFound = true; 
                
                pthread_mutex_lock(&file_queue.mutex);
                file_path = file_queue.queue.front();
                file_queue.queue.pop(); 
                pthread_mutex_unlock(&file_queue.mutex); 
                
                // File size information is needed to dynamically reallocate size of the buffer pool.
                stat64(file_path.c_str(), &sb);
	            file_size = sb.st_size; 
            
                fd = open(file_path.c_str(), O_RDONLY); 
                if (fd == -1)
                    cout << "error opening file | errno: " << errno << endl; 
                test_open_cnt ++;  // passed

                cout << "rank " << rank << ' ' << "a2\n"; 



                // TODO: If file size is over 2GB, we should iteratively read file to the end. 
                uint64_t remaining_file_size = file_size; 
                uint64_t max_read_size = 104857600;
				
				for(uint64_t offset = 0; offset < file_size; offset += max_read_size)
                {

                    cout << "rank " << rank << ' ' << "a30\n"; 
                
                    buffer = &bufferpool[reader_idx]; 
                 
                    pthread_mutex_lock(&buffer->mutex); 
                    while(buffer->filled)
                    {
                        pthread_cond_wait(&buffer->cond, &buffer->mutex); 
                    }

                    uint64_t buffer_size = (max_read_size < remaining_file_size)? max_read_size:remaining_file_size; 
                    buffer->data = (char*)malloc(buffer_size);
                    if (buffer->data == NULL)
                    {
                        cout << "memory alloction error on readerThread errno: " << errno << endl;
                        cout << "requested memory size: " << buffer_size << endl; 
                        // printf("requested memory size: %lld\n", buffer_size); 
                    }
                    remaining_file_size -= buffer_size; 

                    cout << "rank " << rank << ' ' << "a31\n"; 


                    bytesRead = pread(fd, buffer->data, buffer_size, offset);
                    if (bytesRead != buffer_size)
                    {
                        printf("read size is wrong\t%lld\t%lld\n", file_size, bytesRead);                      
                    }
                    if (bytesRead == -1)
                    {
                        perror("error reading file\n"); 
                        cout << "error reading file\n" ; 
                    }
                    else{
                        if (bytesRead >= 0 && bytesRead < buffer_size)
                            buffer->data[bytesRead] = '\0'; 

                        buffer->filled = 1; 
                        buffer->size = bytesRead;

                        reader_idx = (reader_idx + 1) % POOL_SIZE ; 
                    }

                    cout << "rank " << rank << ' ' << "a32\n"; 

                    pthread_cond_signal(&buffer->cond);
                    pthread_mutex_unlock(&buffer->mutex); 
                        
                    cout << "rank " << rank << ' ' << "a33\n"; 
                
                }

                close(fd);
                
                // TestCode
                
                            

                /*
                
                // Hold one single emtpy buffer from buffer pool.
                buffer = &bufferpool[reader_idx];

                pthread_mutex_lock(&buffer->mutex);
                while(buffer->filled)
                {
                    pthread_cond_wait(&buffer->cond, &buffer->mutex); 
                }

                // buffer->data = new char[file_size+1];
                // buffer->data = new char[1073741824+1];
                
                buffer->data = (char * )malloc(file_size);
                if(buffer->data == NULL)
                    cout << "memory allocation error on reader Thread \n";  
                // bytesRead = read(fd, buffer->data, file_size); 
                bytesRead = read(fd, buffer->data, file_size); 
                
                if(bytesRead != file_size)
                {
                    printf("read size is wrong\t%lld\t%lld\n", file_size, bytesRead); 
                }

                if (bytesRead == -1)
                {
                    perror("error reading file");
                    // exit(1); 
                }
                //TODO: Check if there is a case where bytesRead and filesize is different. 
                buffer->filled = 1; 
                if (bytesRead >= 0 && bytesRead < file_size)
                    buffer->data[bytesRead] = '\0'; 
                buffer->size = bytesRead; 
            

                reader_idx = (reader_idx + 1) % POOL_SIZE; 
            
                pthread_cond_signal(&buffer->cond);
                pthread_mutex_unlock(&buffer->mutex); 
*/
            }

            
            
            if (shutdown_flag && !taskFound)
            {
                reader_done = true; 
				
				for (Buffer &b : bufferpool)
				{
					pthread_mutex_lock(&b.mutex); 
					pthread_cond_signal(&b.cond); 
					pthread_mutex_unlock(&b.mutex); 
					if (b.filled)
						printf("Some buffers are filled\n"); 
				}

                printf("reader done: %d\t\n", rank);
                printf("rank%d\topen%d\n", rank, test_open_cnt); 
                
                for(Buffer &b : bufferpool){
                    pthread_mutex_lock(&b.mutex);
                    pthread_cond_signal(&b.cond);
                    pthread_mutex_unlock(&b.mutex);
                    if (b.filled)
                            printf("Some buffers are filled\n"); 
		        }
                
                break; 
            }
            

        
        }                

    }

    void * Dedupe::workerThread(int w_idx)
    {
        static int test_work_cnt = 0; 
        int worker_idx = w_idx; 

        while(1)
        {
            cout << "rank " << rank << ' ' << "b1\n"; 

            Buffer * buffer = &bufferpool[worker_idx]; 
            pthread_mutex_lock(&buffer->mutex); 

            // 버퍼가 비었는데 reader Thread는 아직 돌고 있다: 이 버퍼가 채워질때까지 기다려야 한다.
            while(!buffer->filled & !reader_done)
            {
                pthread_cond_wait(&buffer->cond, &buffer->mutex); 
            }

            cout << "rank " << rank << ' ' << "b2\n"; 


            if (reader_done && !buffer->filled)
            {
                pthread_mutex_unlock(&buffer->mutex);
                break; 
            }
            string str(buffer->data, buffer->size); 
            test_file_cnt += 1; 
            printf("rank %d %lld\n", rank, buffer->size); 
            chunk_fixed_size(str, buffer->size); 
            
            cout << "rank " << rank << ' ' << "b3\n"; 


            test_work_cnt += 1; 
        
            buffer->filled = 0; 
            free(buffer->data); 
            worker_idx = (worker_idx + numWorkers) % POOL_SIZE; 

            pthread_cond_signal(&buffer->cond); 
            pthread_mutex_unlock(&buffer->mutex); 

            cout << "rank " << rank << ' ' << "b4\n"; 


            if (reader_done)
            {
                bool allBuffersDone = true; 

                for(int i = worker_idx; i < POOL_SIZE; i+= numWorkers)
                {
                    if (bufferpool[i].filled){
                        allBuffersDone = false; 
                        break; 
                    }
                }

                if (allBuffersDone)
                {
                    cout << "worker done" <<  rank << endl; 
                    break; 
                }
            }

            cout << "rank " << rank << ' ' << "b5\n";  

        }

        printf("rank\t%d\tchunk_cnt%d\n", rank, test_chunk_cnt);



    } 


//     void* Dedupe::readerThread() {
//       	object_task metadata;
// 	    Buffer * buffer; 
// 	    int ostPerRank = this->ostPerRank;
//         while(1) {
//             bool taskFound = false;
//             for (int i = 0; i < ostPerRank; i++) {
//                 while(!ost_q[i].taskQueue.empty()) { 
//                     taskFound = true;
                    
// 					pthread_mutex_lock(&ost_q[i].mutex);
                    
// 					metadata = ost_q[i].taskQueue.front();
// 					ost_q[i].taskQueue.pop();
					
// 					pthread_mutex_unlock(&ost_q[i].mutex);
// 					int fd = open(metadata.file_path, O_RDONLY);
//                     if (fd == -1) {
//                         perror("open");

// 			            cout << "error opening file errno: " << errno << endl;
//                         break; 
// 						//return NULL;
//                     }
                    
// 					for(uint64_t offset = metadata.start; offset < metadata.end; offset += metadata.interval)			{
			
//                         buffer = &bufferpool[reader_idx];
						
// 						pthread_mutex_lock(&buffer->mutex);
//                 //        while((reader_idx +1)% POOL_SIZE == worker_idx && buffer->filled) {
// 				while(buffer->filled){
// 							cout << "reader waiting for buffer to be emptied " << rank << endl;
//                        		pthread_cond_wait(&buffer->cond, &buffer->mutex);
// 							cout << "reader escaped : " << rank << endl;	
//                         }
// 						//pthread_mutex_unlock(&buffer->mutex);
                    
						
// 						//   if(buffer!= nullptr && buffer->data != nullptr){
// 			            uint64_t size = (offset + metadata.size > metadata.end)? (metadata.end - offset):metadata.size; 
// 			            ssize_t bytesRead = pread(fd, buffer->data, size, offset); 
//                         if (bytesRead == -1) {
//                             perror("Error reading file");
//                             exit(1);
//                         }
//                         //cout << "pread done: bytesRead = " << bytesRead << ", offset = " << offset << endl;
//                         buffer->filled = 1;
//                         if(bytesRead >= 0 && bytesRead < size)
//                             buffer->data[bytesRead] = '\0';
//                         buffer->size = bytesRead;
//                      //   }
                        
						
// 						reader_idx = (reader_idx + 1) % POOL_SIZE;
// 						pthread_cond_signal(&buffer->cond);	
// 						pthread_mutex_unlock(&buffer->mutex);
//                     }
//                     close(fd);
//                 }
//             }// end of for

//             if (shutdown_flag && !taskFound) {
// 				reader_done = true;
// 				printf("reader done %d\n", this->rank); 
// //				pthread_cond_signal(&buffer->cond); 

// 				for(Buffer &b : bufferpool){
// 					pthread_mutex_lock(&b.mutex);
// 					pthread_cond_signal(&b.cond);
// 					pthread_mutex_unlock(&b.mutex);
// 					if (b.filled)
// 						printf("Some buffers are filled\n"); 
// 				}
				
//                 break;
//             }
//         }
// 	return NULL;
//     }


    // void* Dedupe::workerThread(int w_idx) {
	// 	static int work_cnt = 0; 
	// 	int worker_idx = w_idx;
		
	// 	while (1) {
	// 	    Buffer *buffer = &bufferpool[worker_idx];
	// 	    pthread_mutex_lock(&buffer->mutex);
    //         // Wait until buffer is filled
    //     //    		if(worker_idx == reader_idx && !buffer->filled){
	// 		while(!buffer->filled && !reader_done){
	// 			cout << "worker waiting to be filled: " << rank << endl;
	// 			pthread_cond_wait(&buffer->cond, &buffer->mutex);
	// 			cout << "worker woke up!\n"; 
	// 		}
	// 		//pthread_mutex_unlock(&buffer->mutex);
    //         if(reader_done && !buffer->filled){
	// 			pthread_mutex_unlock(&buffer->mutex);
	// 			break;
	// 		}
	// 		string str(buffer->data, buffer->size);
	// 		chunk_fixed_size(str, buffer->size);
	// 		work_cnt += 1; 

    //     	buffer->filled = 0;            	
	// 		worker_idx = (worker_idx + numWorkers) % POOL_SIZE;
			
	// 		pthread_cond_signal(&buffer->cond);
	// 		pthread_mutex_unlock(&buffer->mutex);

	// 		if (reader_done) {
	// 			bool allBuffersDone = true;


	// 		for(int i = worker_idx; i < POOL_SIZE; i += numWorkers){
    //             		if(bufferpool[i].filled){
    //                 			allBuffersDone = false;
    //                 			break;
    //             		}
    //         		}

	// 			if (allBuffersDone){
	// 				cout << "obj cnt: " << obj_cnt << endl;
	// 				cout << "worker done " << rank << endl; 
	// 				printf("work_cnt %d: %d\n", rank, work_cnt); 
	// 				break;
	// 			}
	// 		} 
	// 	}
	//     return NULL;
    // }


    void Dedupe::initialize_queue()
    {
        pthread_mutex_init(&file_queue.mutex, NULL);
        pthread_cond_init(&file_queue.cond, NULL);
        
        //TODO: Buffer Pool Iniitialization  
    }

    void Dedupe::initializeq(int ostPerRank) {
        this->ost_q.resize(ostPerRank);

        for (int i = 0; i < ostPerRank; i++) {
            pthread_mutex_init(&(this->ost_q[i].mutex), NULL);
            pthread_cond_init(&(this->ost_q[i].cond), NULL);
        }
    }

    int Dedupe::traverse_directory(string directory_path){
        cout << "Directory traversing started" << endl;

        // Initialize MPI environment
	    int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

        // Get rank and world size
        int rank, worldSize;
        int unchecked_file = 0;
	    int numWorkers = this->numWorkers;

        // test code
        static int test_regular_file_cnt = 0; 

        MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		this->worldSize = worldSize;
		this->rank = rank;
        cout << "Current process rank: " << this->rank << endl;

        if (rank == MASTER){
	        
            vector<File_task_queue> file_task_queue(worldSize-1); 
			vector<pair<string, uint64_t>> file_task_list;  // Load Balance			
			
            // test purpose code: load_balance test 
			size_per_rank = (uint64_t *)malloc(sizeof(uint64_t) * worldSize); 
			if (size_per_rank == NULL)
			{
				cout << "memory allocation error on size_per_rank";
				exit(1); 
			}
			memset(size_per_rank, 0, sizeof(uint64_t) * worldSize); 
			
			
			auto start = chrono::high_resolution_clock::now();
			
			for (const auto &dir_entry: filesystem::recursive_directory_iterator(directory_path)){
                total_file ++ ;

                if(filesystem::is_symlink(dir_entry)){
                    // cout << "Symlink encountered\n"; 
                    continue; 
                }
				
                if(dir_entry.is_regular_file()){
                    test_regular_file_cnt += 1; 
				    file_task_distribution(dir_entry, file_task_queue, file_task_list); 
                    continue;
                }

                unchecked_file ++ ; 
            }
		

			if (load_balance)
			{
                file_task_load_balance(file_task_list); 
				//object_task_load_balance(task_queue); f
			}
			
			printf("unchecked file: %d/%d\n", unchecked_file, total_file); 
            printf("user-configured PFL file: %d\n", this->non_default_pfl); 
            printf("total_file_size: %lld\n", total_file_size); 
			
            printf("task_cnt_sent: %d\n", test_regular_file_cnt);

//			printf("master task_cnt: %lld\n", this->task_cnt);
            
            file_task_end_of_process(file_task_queue); 
        }
        else{
            // Mutex initialization
            for (int i = 0; i < POOL_SIZE; i++) {
				bufferpool[i].filled = 0;
                pthread_mutex_init(&bufferpool[i].mutex, NULL);
                pthread_cond_init(&bufferpool[i].cond, NULL);
            }

            initialize_queue(); 


            int ostPerRank = OST_NUMBER / (worldSize-1);
            int remainder = OST_NUMBER % (worldSize-1);
            if(rank < remainder){
                ostPerRank++;
            }
            this->ostPerRank = ostPerRank;
            this->initializeq(ostPerRank);	    

            pthread_t comm, reader, worker;

            
            int rc1 = pthread_create(&comm, NULL, Dedupe::commThreadStarter, this);
            if(rc1) {
                cout << "error: thread creation " << rc1 << endl;
                exit(-1);
            }
    
            // int rc2 = pthread_create(&reader, NULL, Dedupe::readerThreadStarter, this);
            // if(rc2) {
            //     cout << "error: thread creation " << rc2 << endl;
            //     exit(-1);
            // }
	
		printf("numworkers:%d\n", numWorkers) ;
		printf("loadbalance: %d\n", load_balance);
	    vector<pthread_t> workerThreads(numWorkers);
        vector<ThreadArgs> threadArgs(numWorkers);	
		
	    // for(int i = 0; i < numWorkers; i++) {
        //         threadArgs[i].instance = this;
        //         threadArgs[i].index = i;

        //         int rc = pthread_create(&workerThreads[i], NULL, Dedupe::workerThreadStarter, &threadArgs[i]);
        //         if(rc) {
        //             cout << "error: thread creation" << rc << endl;
        //             exit(-1);
        //         }
        //     }	

        

            (void)pthread_join(comm, NULL);
		
        	// (void)pthread_join(reader, NULL);

   //       (void)pthread_join(worker, NULL);
    
		// for(int i = 0; i < numWorkers; i++) {
        //         int rc = pthread_join(workerThreads[i], NULL);
        //         if (rc){
        //             cout << "error: thread join" << rc << endl;
        //             exit(-1);
        //         }
        // }		
	
        }
	
        // Finalize MPI environment
        MPI_Finalize();

        return rank;

    }
    void Dedupe::chunk_fixed_size(const string &buffer, uint64_t obj_size){
        
        int test_single_chunk_cnt = 0; 
        int test_is_last_chunk = 0; 

        uint64_t o_size = obj_size;
		this->obj_cnt++;
        if (buffer.empty() || o_size == 0){
            return;
        }
        uint64_t o_pos = 0;
        

        vector<string> fingerprints;
        //cout << "start fingerprinting" << endl;
        while (o_pos < o_size){


            cout << "rank " << rank << ' ' << "k00\n"; 

        
	        uint64_t cur_chunk_size = min(static_cast<uint64_t>(chunk_size), o_size -o_pos);
            string chunk = buffer.substr(o_pos, cur_chunk_size);
	        if(cur_chunk_size < chunk_size){
		        chunk += string(chunk_size - cur_chunk_size, '0');
                test_is_last_chunk = 1; 
	        }   	
	        o_pos += chunk_size;

            cout << "rank " << rank << ' ' << "k01"; 
            printf(" %d %lld %d %d %d %d\n", test_file_cnt, obj_size, test_single_chunk_cnt, cur_chunk_size, chunk_size);
            cout << "";


            unsigned char temp_fp[SHA_DIGEST_LENGTH];
            SHA1(reinterpret_cast<const unsigned char *>(chunk.c_str()), chunk_size, temp_fp);
            test_chunk_cnt ++; 
            test_single_chunk_cnt ++; 
        
            cout << "rank " << rank << ' ' << "k02\n"; 

            string fp = GetHexRepresentation(temp_fp, SHA_DIGEST_LENGTH);

            fingerprints.push_back(fp);

        }

        cout << "rank " << rank << ' ' << "k1\n"; 

        // Create the fingerprint string
        string all_fingerprints;
        for (const string &fp : fingerprints)
        {
            all_fingerprints += fp + "\n";
        }
        string output_file = this->output_file.c_str() + to_string(this->rank) + ".txt";
        //cout << "output_file = " << output_file << endl;
        // Open the output file

        cout << "rank " << rank << ' ' << "k1\n"; 


        ofstream ofs(output_file, ios::app);
        if (!ofs){
            cerr << "Error opening output file\n";
            exit(0);
        }
        ofs << all_fingerprints;
        ofs.close();

        cout << "rank " << rank << ' ' << "k2\n"; 

    
        

    }

};

int main(int argc, char **argv)
{
    uint64_t chunk_size = 0;
    string directory_path;
    string output_file;
    int chunk_mode = 0;
    int c;
    int fstat_flag = 0;
    int rank;
    int numWorkers = 1;
	int load_balance = 1; 
    while ((c = getopt(argc, argv, "f:s:m:i:o:t:l:")) != -1)
    {
        switch (c)
        {
        case 'f':
            fstat_flag = atoi(optarg);
            break;
        case 's':
            chunk_size = atoi(optarg);
            break;
        case 'm':
            chunk_mode = atoi(optarg);
            break;
        case 'i':
            directory_path = optarg;
            break;
        case 'o':
            output_file = optarg;
			break; 
		case 't':
			numWorkers = atoi(optarg);	    
            break;
		case 'l':
			load_balance = atoi(optarg);	    
            break;
        default:
            break;
        }
    }
    auto start = chrono::high_resolution_clock::now();
    if (fstat_flag == 1)
    {
        danzer::FStat *fstat = new danzer::FStat();
        ofstream fstat_file;

        fstat_file.open(output_file, ios::out);
        if (!fstat_file)
        {
            cout << "Out file error" << endl;
            exit(0);
        }

        fstat->measure_file_sizes(directory_path, fstat_file);

        cout << endl;
        cout << "File Statistics session end . . ." << endl;

        return 0;
    }

    else if (fstat_flag == 2)
    {
        danzer::FStat *fstat = new danzer::FStat();
        ofstream fstat_file;

        fstat_file.open(output_file, ios::out);
        if (!fstat_file)
        {
            cout << "Out file error" << endl;
            exit(0);
        }

        fstat->measure_cumulative_fs(directory_path, fstat_file);

        cout << endl;
        cout << "* Cumulative distribution session end . . ." << endl;

        return 0;
    }

    else if (fstat_flag == 3)
    {
        danzer::FStat *fe = new danzer::FStat();
        ofstream fe_file;

        fe_file.open(output_file, ios::out);
        if (!fe_file)
        {
            cout << "Out file error" << endl;
            exit(0);
        }

        fe->measure_file_extensions(directory_path, fe_file); ////////////////////////// TODO!! start here

        cout << endl;
        cout << "** Measuring file extensions session end . . ." << endl;

        return 0;
    }

    else
    {
        danzer::Dedupe *dedup = new danzer::Dedupe(chunk_mode, chunk_size, 0, output_file, numWorkers, load_balance);

        rank = dedup->traverse_directory(directory_path);
    }
    auto end = chrono::high_resolution_clock::now();

    // Calculate the duration
    chrono::duration<double> duration = end - start;

    // Print the execution time
	
	if (rank == 0)
	{
		FILE * fp = fopen("exec_time.txt", "a"); 
		if (fp == NULL)
			printf("file open error\n"); 
		
		fprintf(fp, "exec_time\t%lf\n", duration.count()); 
        cout << "Execution time: " << duration.count() << " seconds" << endl;
	}
	return 0;

}
