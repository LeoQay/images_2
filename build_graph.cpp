#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <set>
#include <map>


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

void zero_borders(Image & image)
{
    auto ptr = image.data;
    for (int64_t i = 0; i < image.shape[1]; i++)
    {
        ptr[i] = 0;
    }

    ptr = image.data + (image.shape[0] - 1) * image.shape[1];
    for (int64_t i = 0; i < image.shape[1]; i++)
    {
        ptr[i] = 0;
    }

    ptr = image.data;
    for (int64_t i = 0; i < image.shape[0]; i++)
    {
        *ptr = 0;
        ptr += image.shape[1];
    }

    ptr = image.data + image.shape[1] - 1;
    for (int64_t i = 0; i < image.shape[0]; i++)
    {
        *ptr = 0;
        ptr += image.shape[1];
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



struct Graph
{
    void add_edge(int64_t id1, int64_t id2, int64_t length, int64_t info1, int64_t info2)
    {
        int64_t a = std::min(id1, id2);
        int64_t b = std::max(id1, id2);
        int64_t c = std::min(info1, info2);
        int64_t d = std::max(info1, info2);
        edges.emplace(a, b, length, c, d);
    }

    std::set<int64_t> vertexes;
    std::set<std::tuple<int64_t, int64_t, int64_t, int64_t, int64_t>> edges;
};


void process_graph_edge(
    Graph & graph,
    const Image & image,
    int64_t start,
    int64_t dest)
{
    auto data = image.data;
    int64_t rows = image.shape[0];
    int64_t cols = image.shape[1];

    int64_t current = start;
    int64_t previous = -1;
    int64_t len = 0;

    int64_t first_edge_pixel = -1;
    int64_t second_edge_pixel = -1;

    if (dest != -1)
    {
        current = dest;
        previous = start;
        len = 1;
        first_edge_pixel = current;
    }

    bool n[3][3];
    n[1][1] = false;

    while (true)
    {
        n[0][1] = data[current - cols];
        n[1][0] = data[current - 1];
        n[1][2] = data[current + 1];
        n[2][1] = data[current + cols];
        
        n[0][0] = data[current - cols - 1] && (!n[1][0]) && (!n[0][1]);
        n[0][2] = data[current - cols + 1] && (!n[0][1]) && (!n[1][2]);
        n[2][0] = data[current + cols - 1] && (!n[1][0]) && (!n[2][1]);
        n[2][2] = data[current + cols + 1] && (!n[2][1]) && (!n[1][2]);

        if (previous != -1)
        {
            int64_t prev_i = 1 + (previous / cols) - (current / cols);
            int64_t prev_j = 1 + (previous % cols) - (current % cols);
            n[prev_i][prev_j] = false;
        }

        std::vector<int64_t> n_indexes;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (n[i][j])
                {
                    n_indexes.push_back(current + (i - 1) * cols + (j - 1));
                }
            }
        }

        len += 1;

        if (n_indexes.size() == 1)
        {
            if (first_edge_pixel == -1) first_edge_pixel = n_indexes[0];
            previous = current;
            current = n_indexes[0];
        }
        else
        {
            if (len == 1) {
                first_edge_pixel = -1;
                second_edge_pixel = -1;
            } else {
                second_edge_pixel = previous;
            }

            graph.add_edge(start, current, len, first_edge_pixel, second_edge_pixel);

            bool was_processed = graph.vertexes.find(current) != graph.vertexes.end();

            if (!was_processed)
            {
                graph.vertexes.insert(current);

                for (auto index : n_indexes)
                {
                    process_graph_edge(graph, image, current, index);
                }
            }
            
            return;
        }
    }
}


Graph build_graph(const Image & image, int64_t start)
{
    Graph graph;
    graph.vertexes.insert(start);
    process_graph_edge(graph, image, start, -1);
    return graph;
}


std::vector<Graph> build_graphs(const Image & image)
{
    auto data = image.data;
    int64_t rows = image.shape[0], cols = image.shape[1];

    std::vector<Graph> graphs;

    std::set<int64_t> vertexes;

    bool n[3][3];
    n[1][1] = false;

    // find all vertexes
    for (int64_t i = 1; i < rows - 1; i++)
    {
        for (int64_t j = 1; j < cols - 1; j++)
        {
            int64_t index = i * cols + j;
            
            if (data[index] == 0) continue;
            
            n[0][1] = data[index - cols];
            n[1][0] = data[index - 1];
            n[1][2] = data[index + 1];
            n[2][1] = data[index + cols];
        
            n[0][0] = data[index - cols - 1] && (!n[1][0]) && (!n[0][1]);
            n[0][2] = data[index - cols + 1] && (!n[0][1]) && (!n[1][2]);
            n[2][0] = data[index + cols - 1] && (!n[1][0]) && (!n[2][1]);
            n[2][2] = data[index + cols + 1] && (!n[2][1]) && (!n[1][2]);

            int count_neighbors = 0;
            for (int i_ = 0; i_ < 3; i_++) for (int j_ = 0; j_ < 3; j_++) count_neighbors += n[i_][j_];

            if (count_neighbors != 2) vertexes.insert(index);
        }
    }

    while (!vertexes.empty())
    {
        int start = *vertexes.begin();
        vertexes.erase(vertexes.begin());
        graphs.push_back(build_graph(image, start));

        const Graph & graph = graphs.back();

        for (auto index : graph.vertexes)
        {
            auto it = vertexes.find(index);
            if (it != vertexes.end())
            {
                vertexes.erase(it);
            }
        }
    }

    return graphs;
}


void write_graphs(const std::vector<Graph> & graphs, const char * filename)
{
    FILE * file = fopen(filename, "wb");

    int64_t count_graphs = (int64_t) graphs.size();
    fwrite(&count_graphs, sizeof(int64_t), 1, file);

    for (const Graph & graph : graphs)
    {
        int64_t count_vertexes = (int64_t) graph.vertexes.size();
        fwrite(&count_vertexes, sizeof(int64_t), 1, file);

        std::vector<int64_t> buffer1(graph.vertexes.begin(), graph.vertexes.end());
        fwrite(buffer1.data(), sizeof(int64_t), count_vertexes, file);

        int64_t count_edges = (int64_t) graph.edges.size();
        fwrite(&count_edges, sizeof(int64_t), 1, file);
        int64_t buffer2[3];
        for (auto [first, second, len, i1, i2] : graph.edges)
        {
            buffer2[0] = first;
            buffer2[1] = second;
            buffer2[2] = len;
            fwrite(buffer2, sizeof(int64_t), 3, file);
        }
    }

	fclose(file);
}


int main(int argc, char ** argv)
{
    Image image = read_image(argv[1]);
    
    bin_image(image, 1);

    zero_borders(image);

    auto graphs = build_graphs(image);

    write_graphs(graphs, argv[2]);

    delete_image(image);

    return 0;
}

