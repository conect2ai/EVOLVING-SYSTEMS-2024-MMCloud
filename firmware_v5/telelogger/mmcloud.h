#ifndef MMCLOUD_H
#define MMCLOUD_H

#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>  // Inclui a função accumulate
#include <algorithm> // Para std::remove

using namespace std;

class Cluster {
public:
    int id;
    int dimension;
    int count;
    vector<double> sum;
    vector<double> sum_squares;
    vector<double> mean;
    vector<double> variance;
    vector<double> cv;
    string label;  // Pode ser "cauteloso", "normal", "agressivo"

    Cluster(int id, int dimension) {
        this->id = id;
        this->dimension = dimension;
        this->count = 0;
        this->sum = vector<double>(dimension, 0.0);
        this->sum_squares = vector<double>(dimension, 0.0);
        this->mean = vector<double>(dimension, 0.0);
        this->variance = vector<double>(dimension, 0.0);
        this->cv = vector<double>(dimension, 0.0);
        this->label = "None";
    }

    void add_point(const vector<double>& point) {
        count++;
        vector<double> old_mean = mean;
        for (int i = 0; i < dimension; ++i) {
            mean[i] = old_mean[i] + (point[i] - old_mean[i]) / count;
        }
        if (count > 1) {
            for (int i = 0; i < dimension; ++i) {
                variance[i] = ((count - 2) * variance[i] + (point[i] - old_mean[i]) * (point[i] - mean[i])) / (count - 1);
            }
        } else {
            variance = vector<double>(dimension, 0.0);
        }

        // Atualizar o coeficiente de variação
        for (int i = 0; i < dimension; ++i) {
            if (mean[i] != 0) {
                cv[i] = sqrt(variance[i]) / mean[i];
            } else {
                cv[i] = 0.0;
            }
        }
    }

    // Definir o operador de igualdade para comparar clusters
    bool operator==(const Cluster& other) const {
        return this->id == other.id;
    }
};

class MMCloud {
public:
    int dimension;
    int max_clusters;
    int cluster_id_counter;
    int n_distances;
    double mean_distance;
    double variance_distance;
    vector<Cluster> clusters;
    // map<int, pair<int, string>> point_assignments;

    MMCloud(int dimension, int max_clusters = 3) {
        this->dimension = dimension;
        this->max_clusters = max_clusters;
        this->cluster_id_counter = 1;
        this->n_distances = 0;
        this->mean_distance = 0.0;
        this->variance_distance = 0.0;
        Cluster initial_cluster(cluster_id_counter, dimension);
        clusters.push_back(initial_cluster);
        cluster_id_counter++;
    }

    void update_mean_and_variance(double new_distance) {
        n_distances++;
        double old_mean = mean_distance;
        mean_distance += (new_distance - mean_distance) / n_distances;
        variance_distance += (new_distance - old_mean) * (new_distance - mean_distance);
    }

    double calculate_dynamic_outlier_threshold() {
        if (n_distances > 1) {
            double std_distance = sqrt(variance_distance / (n_distances - 1));
            return mean_distance + 2 * std_distance;
        } else {
            return numeric_limits<double>::infinity();
        }
    }

    double calculate_dynamic_dispersion_threshold() {
        if (n_distances > 1) {
            double std_distance = sqrt(variance_distance / (n_distances - 1));
            return mean_distance + 2 * std_distance;
        } else {
            return 1.0;
        }
    }

    pair<Cluster, Cluster> split_cluster_with_variance(Cluster& cluster) {
        int max_var_dimension = 0;
        for (int i = 1; i < cluster.dimension; i++) {
            if (cluster.variance[i] > cluster.variance[max_var_dimension]) {
                max_var_dimension = i;
            }
        }

        double mean_value = cluster.mean[max_var_dimension];
        double variance_value = cluster.variance[max_var_dimension];

        Cluster cluster1(cluster_id_counter, dimension);
        Cluster cluster2(cluster_id_counter + 1, dimension);
        cluster_id_counter += 2;

        cluster1.mean = cluster.mean;
        cluster2.mean = cluster.mean;

        cluster1.mean[max_var_dimension] = mean_value - 0.5 * variance_value;
        cluster2.mean[max_var_dimension] = mean_value + 0.5 * variance_value;

        for (int i = 0; i < dimension; ++i) {
            if (cluster1.mean[i] < 0) {
                cluster1.mean[i] = 0;
            }
            if (cluster2.mean[i] < 0) {
                cluster2.mean[i] = 0;
            }
        }

        return make_pair(cluster1, cluster2);
    }

    string process_point(int point_index, const vector<double>& point) {
        double min_distance = numeric_limits<double>::infinity();
        Cluster* assigned_cluster = nullptr;
        for (Cluster& cluster : clusters) {
            double distance = 0.0;
            for (int i = 0; i < dimension; i++) {
                distance += pow(point[i] - cluster.mean[i], 2);
            }
            distance = sqrt(distance);
            if (distance < min_distance) {
                min_distance = distance;
                assigned_cluster = &cluster;
            }
        }

        // cout << "min_distance: " << min_distance << endl;

        update_mean_and_variance(min_distance);

        double dynamic_outlier_threshold = calculate_dynamic_outlier_threshold();

        if (min_distance > dynamic_outlier_threshold && clusters.size() < max_clusters) {
            Cluster new_cluster(cluster_id_counter, dimension);
            new_cluster.add_point(point);
            clusters.push_back(new_cluster);
            cluster_id_counter++;
            // point_assignments[point_index] = make_pair(new_cluster.id, new_cluster.label);
        } else if (min_distance > dynamic_outlier_threshold) {
            // point_assignments[point_index] = make_pair(-1, "None");
        } else {
            assigned_cluster->add_point(point);
            // point_assignments[point_index] = make_pair(assigned_cluster->id, assigned_cluster->label);
        }

        double dispersion_threshold = calculate_dynamic_dispersion_threshold();

        if (accumulate(assigned_cluster->variance.begin(), assigned_cluster->variance.end(), 0.0) > dispersion_threshold
            && clusters.size() < max_clusters) {
            auto [cluster1, cluster2] = split_cluster_with_variance(*assigned_cluster);
            clusters.erase(remove(clusters.begin(), clusters.end(), *assigned_cluster), clusters.end());
            clusters.push_back(cluster1);
            clusters.push_back(cluster2);
        }

        update_label();

        return assigned_cluster->label;
        // return assigned_cluster->id;
    }

    void update_label() {
        if (clusters.size() == 1) {
            clusters[0].label = "normal";
        } else if (clusters.size() == 2) {
            double dist1 = 0.0, dist2 = 0.0;
            for (int i = 0; i < dimension; ++i) {
                dist1 += pow(clusters[0].mean[i], 2);
                dist2 += pow(clusters[1].mean[i], 2);
            }
            dist1 = sqrt(dist1);
            dist2 = sqrt(dist2);

            if (dist1 < dist2) {
                // //verify the dimension 1 of the cluster
                // if (clusters[0].mean[1] < 40){
                //     clusters[0].label = "cauteloso";

                //     if (clusters[1].mean[1] < 70){
                //         clusters[1].label = "normal";
                //     } else {
                //         clusters[1].label = "agressivo";
                //     }
                // } else if (clusters[0].mean[1] < 70){
                //     clusters[0].label = "normal";
                //     clusters[1].label = "agressivo";
                // }

                clusters[0].label = "cautious";
                clusters[1].label = "aggressive";
            } else {
                //verify the dimension 1 of the cluster
                // if (clusters[1].mean[1] < 40){
                //     clusters[1].label = "cauteloso";

                //     if (clusters[0].mean[0] < 70){
                //         clusters[0].label = "normal";
                //     } else {
                //         clusters[0].label = "agressivo";
                //     }
                // } else if (clusters[1].mean[1] < 70){
                //     clusters[1].label = "normal";
                //     clusters[0].label = "agressivo";
                // }

                clusters[1].label = "cautious";
                clusters[0].label = "aggressive";
            }
        } else {
            vector<double> distances(clusters.size());
            for (int i = 0; i < clusters.size(); ++i) {
                distances[i] = 0.0;
                for (int j = 0; j < dimension; ++j) {
                    distances[i] += pow(clusters[i].mean[j], 2);
                }
                distances[i] = sqrt(distances[i]);
            }

            int cautious_index = min_element(distances.begin(), distances.end()) - distances.begin();
            int aggressive_index = max_element(distances.begin(), distances.end()) - distances.begin();
            int normal_index = 3 - cautious_index - aggressive_index;

            clusters[cautious_index].label = "cautious";
            clusters[aggressive_index].label = "aggressive";
            clusters[normal_index].label = "normal";
        }
    }

    // display clusters information
    void display_clusters() {
        for (Cluster& cluster : clusters) {
            cout << "Cluster " << cluster.id << " (" << cluster.label << "):" << endl;
            cout << "Mean: ";
            for (double mean_value : cluster.mean) {
                cout << mean_value << " ";
            }
            cout << endl;
            cout << "Variance: ";
            for (double variance_value : cluster.variance) {
                cout << variance_value << " ";
            }
            cout << endl;
            cout << "CV: ";
            for (double cv_value : cluster.cv) {
                cout << cv_value << " ";
            }
            cout << endl;
            cout << "Count: " << cluster.count << endl;
            cout << endl;
        }
    }
};

#endif  // MMCLOUD_H
