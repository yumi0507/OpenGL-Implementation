#include "trianglemesh.h"

// Constructor of a triangle mesh.
TriangleMesh::TriangleMesh()
{
	// -------------------------------------------------------
	vboId = 0;
	numVertices = 0;
	numTriangles = 0;
	objCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	objExtent = glm::vec3(0.0f, 0.0f, 0.0f);
	// -------------------------------------------------------
}

// Destructor of a triangle mesh.
TriangleMesh::~TriangleMesh()
{
	// -------------------------------------------------------
	glDeleteBuffers(1, &vboId);
	for (auto&& subMesh : subMeshes) {
		glDeleteBuffers(1, &(subMesh.iboId));
	}
	vertices.clear();
	subMeshes.clear();
	// -------------------------------------------------------
}

// Load the geometry and material data from an OBJ file.
bool TriangleMesh::LoadFromFile(const std::string& model, const bool normalized)
{
	std::cout << "------------------------------" << std::endl;
	std::cout << "Model: " << model << "." << std::endl;
	std::cout << "------------------------------" << std::endl;

	// Set paths.
	std::string folderPath = "./TestModels_HW3/" + model + "/";
	std::string objPath = folderPath + model + ".obj";
	std::string mtlPath;

	// Read a line of an opened file.
	std::string line;

	// Load *.obj file.
	std::ifstream objFile;
	objFile.open(objPath);
	if (!objFile.is_open()) {
		std::cout << "Fail to open the *.obj file.\n" << std::endl;
		return false;
	}

	int vertexIndex = 0;
	// Determine whether P, T, or N.
	std::vector<glm::vec3> Position;
	std::vector<glm::vec2> Texture;
	std::vector<glm::vec3> Normal;
	std::vector<std::vector<int>> face;
	std::string indexSet;	// Read the ptnIndex temporarily.

	while (getline(objFile, line)) {
		std::istringstream read(line);

		std::string type;
		read >> type;
		switch (type[0]) {
		case 'm': {
			read >> type;
			mtlPath = folderPath + type;
			break;
		}
		case 'u':
		{
			read >> type;	// Read the material's type.
			SubMesh newMesh;
			newMesh.material = new PhongMaterial;
			newMesh.material->SetName(type);
			subMeshes.push_back(newMesh);
			break;
		}
		case 'v':
		{
			if (type.size() == 1) {			// position
				glm::vec3 p;
				read >> p[0] >> p[1] >> p[2];
				Position.push_back(p);
			}
			else if (type[1] == 't') {		// texture coordinate
				glm::vec2 t;
				read >> t[0] >> t[1];
				Texture.push_back(t);
			}
			else if (type[1] == 'n') {		// normal
				glm::vec3 n;
				read >> n[0] >> n[1] >> n[2];
				Normal.push_back(n);
			}
			break;
		}

		case 'f':	// Position / TextureCoordinate / Normal  Indices
		{
			while (read >> indexSet) {
				char temp;
				std::istringstream ptnSet(indexSet);

				int p, t, n;
				std::vector<int> pnt;
				ptnSet >> p >> temp >> t >> temp >> n;
				pnt.push_back(--p);
				pnt.push_back(--n);
				pnt.push_back(--t);
				face.push_back(pnt);
			}

			// Fill in vertex attributes. 
			for (const auto& pnt : face) {
				vertices.push_back(VertexPTN(Position[pnt[0]], Normal[pnt[1]], Texture[pnt[2]]));
			}

			// Polygon Subdivision.
			for (int i = 0; i < face.size() - 2; i++)
			{
				subMeshes[subMeshes.size() - 1].vertexIndices.push_back(vertexIndex);
				subMeshes[subMeshes.size() - 1].vertexIndices.push_back(vertexIndex + i + 1);
				subMeshes[subMeshes.size() - 1].vertexIndices.push_back(vertexIndex + i + 2);
			}
			vertexIndex += face.size();

			face.clear();

			break;
		}

		default:
			break;

		}
	}
	objFile.close();
	// ---------------------------------------------------------------------------
	/*
	// Print out vertices.
	for (int i = 0; i < vertices.size(); i++) {
		std::cout << "No." << i << std::endl;
		std::cout << "position: " << vertices[i].position.x << " " << vertices[i].position.y << " " << vertices[i].position.z << std::endl;
		std::cout << "normal: " << vertices[i].normal.x << " " << vertices[i].normal.y << " " << vertices[i].normal.z << std::endl;
		std::cout << "texcoord: " << vertices[i].texcoord.x << " " << vertices[i].texcoord.y << std::endl;
		std::cout << std::endl;
	}
	// Print out subMeshe.indices
	for (int i = 0; i < subMeshes.size(); i++) {
		std::cout << "Name: " << subMeshes[i].material->GetName() << std::endl;
		for (const auto& indices : subMeshes[i].vertexIndices) {
			std::cout << indices << " ";
		}std::cout << std::endl;
	}
	*/
	// ---------------------------------------------------------------------------


	// Counting Vertices and Triangles.
	numVertices = (int)vertices.size();
	for (const auto& mesh : subMeshes) {
		numTriangles += mesh.vertexIndices.size();
	}
	numTriangles /= 3;
	// ---------------------------------------------------------------------------

	// Bounding box.
	glm::vec3 minBound(std::numeric_limits<float>::max());
	glm::vec3 maxBound(std::numeric_limits<float>::lowest());

	for (const VertexPTN& vertex : vertices) {
		minBound = glm::min(minBound, vertex.position);
		maxBound = glm::max(maxBound, vertex.position);
	}
	// ---------------------------------------------------------------------------

	// Normalize the geometry data.
	glm::vec3 Scale(1.0f);
	glm::vec3 modelSize;
	glm::vec3 modelCenter;
	if (normalized) {
		// -----------------------------------------------------------------------
		// Add your normalization code here (HW1).
		// Normalize the geometry data.
		modelSize = maxBound - minBound;
		modelCenter = (minBound + maxBound) * 0.5f;

		float maxDimension = std::max(std::max(modelSize.x, modelSize.y), modelSize.z);

		if (maxDimension > 0.0f) {
			Scale = glm::vec3(1.0f) / maxDimension;
		}

		for (auto& vertex : vertices) {
			vertex.position = (vertex.position - modelCenter) * Scale;
		}
		// -----------------------------------------------------------------------
	}

	// Load *.mtl file.
	std::ifstream mtlFile;
	mtlFile.open(mtlPath);
	if (!mtlFile.is_open()) {
		std::cout << "Fail to open the *.mtl file.\n" << std::endl;
		return false;
	}

	PhongMaterial* currMaterial{};

	while (getline(mtlFile, line)) {
		std::istringstream read(line);

		std::string temp;
		read >> temp;
		if(temp == "newmtl")
		{
			read >> temp;
			for (int i = 0; i < subMeshes.size(); i++) {
				if (subMeshes[i].material->GetName() == temp) {
					currMaterial = subMeshes[i].material;
					break;
				}
			}
		}
		if(temp == "Ns")
		{
			float ns;
			read >> ns;
			currMaterial->SetNs(ns);
		}
		if (temp == "Ka") {
			glm::vec3 ka;
			read >> ka[0] >> ka[1] >> ka[2];
			currMaterial->SetKa(ka);
		}
		if (temp == "Kd") {
			glm::vec3 kd;
			read >> kd[0] >> kd[1] >> kd[2];
			currMaterial->SetKd(kd);
		}
		if (temp == "Ks") {
			glm::vec3 ks;
			read >> ks[0] >> ks[1] >> ks[2];
			currMaterial->SetKs(ks);
		}
		if(temp == "map_Kd")
		{
			read >> temp;
			if (temp == "-o") 
				read >> temp >> temp >> temp >> temp;
			ImageTexture* image = new ImageTexture(folderPath + temp);
			currMaterial->SetMapKd(image);
		}

	}
	mtlFile.close();

	/*
	// Print out subMesh material.
	for (const auto& mtl : subMeshes) {
		std::cout << "Name: " << mtl.material->GetName() << std::endl;
		std::cout << "Ka " << mtl.material->GetKa().x << " " << mtl.material->GetKa().y << " " << mtl.material->GetKa().z << std::endl;
		std::cout << "Kd " << mtl.material->GetKd().x << " " << mtl.material->GetKd().y << " " << mtl.material->GetKd().z << std::endl;
		std::cout << "Ks " << mtl.material->GetKs().x << " " << mtl.material->GetKs().y << " " << mtl.material->GetKs().z << std::endl;
		std::cout << "Ns " << mtl.material->GetNs() << std::endl;
		std::cout << std::endl;
	}
	*/
	// -----------------------------------------------------------------------

	return true;
}

// Create Buffers.
void TriangleMesh::CreateBuffers()
{
	// Generate the vertex buffer.
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPTN) * numVertices, &vertices[0], GL_STATIC_DRAW);

	// Generate the index buffer.
	for (auto& sub : subMeshes) {
		glGenBuffers(1, &(sub.iboId));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * sub.vertexIndices.size(), &sub.vertexIndices[0], GL_STATIC_DRAW);
	}
}

// Render each subMesh.
void TriangleMesh::RenderSubMesh(SubMesh subMesh)
{
	// Render the triangle mesh.
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)12);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)24);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.iboId);
	glDrawElements(GL_TRIANGLES, (int)subMesh.vertexIndices.size(), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

// Show model information.
void TriangleMesh::ShowInfo()
{
	std::cout << "# Vertices: " << numVertices << std::endl;
	std::cout << "# Triangles: " << numTriangles << std::endl;
	std::cout << "Total " << subMeshes.size() << " subMeshes loaded" << std::endl;
	for (unsigned int i = 0; i < subMeshes.size(); ++i) {
		const SubMesh& g = subMeshes[i];
		std::cout << "SubMesh " << i << " with material: " << g.material->GetName() << std::endl;
		std::cout << "Num. triangles in the subMesh: " << g.vertexIndices.size() / 3 << std::endl;
	}
	std::cout << "Model Center: " << objCenter.x << ", " << objCenter.y << ", " << objCenter.z << std::endl;
	std::cout << "Model Extent: " << objExtent.x << " x " << objExtent.y << " x " << objExtent.z << std::endl;
}

