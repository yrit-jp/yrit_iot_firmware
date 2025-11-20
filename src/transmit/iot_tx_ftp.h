#ifndef MY_FTP_H
#define MY_FTP_H

#include "iot_tx.h"
#include <Arduino_FTPClient.h>

class IoTTxFTP : public IoTTx {
public:
	/*! ==============================================================
	 * @brief FTP 送信に必要なパラメータを設定します
	 * @param[in] (server) 接続先サーバアドレス
	 * @param[in] (user) ログインユーザ名
	 * @param[in] (pass) ログインパスワード
	 * @param[in] (ftp_verbose) ログ出力の粒度（0:無し, 1:最低, 2:最高）
	 * ============================================================= */
	void setFtpParam(String server, String user, String pass, uint8_t ftp_verbose = 1);

	/*! ==============================================================
	 * @brief 送信データを保存するパスを設定します
	 * @param[in] (current_dir) 送信先ルートディレクトリ
	 * @param[in] (file_name) 保存ファイル名
	 * ============================================================= */
	void setFilePath(String current_dir, String file_name);

	/*! ==============================================================
	 * @brief ハートビートデータを保存するパスを設定します
	 * @param[in] (current_dir) 送信先ルートディレクトリ
	 * @param[in] (file_name) 保存ファイル名
	 * ============================================================= */
	void setHeartbeatFilePath(String current_dir, String file_name);

	/*! ==============================================================
	 * @brief データを FTP サーバに送信します
	 * @param[in] (dir) 送信先ディレクトリ（ルートディレクトリ以下）
	 * @param[in] (file_name) 保存ファイル名
	 * @param[in] (value) 送信データ
	 * @param[in] (create_new) ファイルを新規作成するか（false の場合は追記）
	 * @return 送信成功の場合は true, 失敗の場合は false
	 * ============================================================= */
	bool txFtp(String dir, String file_name, String value, bool create_new = false);

	/*! ==============================================================
	 * @brief 画像データを FTP サーバに送信します
	 * @param[in] (dir) 送信先ディレクトリ（ルートディレクトリ以下）
	 * @param[in] (file_name) 保存ファイル名
	 * @param[in] (jpgImage) 送信画像データ
	 * @return 送信成功の場合は true, 失敗の場合は false
	 * ============================================================= */
	bool uploadImage(String dir, String file_name, std::vector<uint8_t> jpgImage);

	/*! ==============================================================
	 * @brief ハートビートデータを送信します
	 * @return 送信成功の場合は true, 失敗の場合は false
	 * ============================================================= */
	bool transmitHeartbeat() override;

protected:
	/*! ==============================================================
	 * @brief 送信処理を行います（通常のセンサデータ用）
	 * @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	 * ============================================================= */
	IoTTx_Tx_Result transmit() override;

	/*! ==============================================================
	 * @brief 送信処理を行います（特殊なセンサデータ用）
	 * @return 送信結果に応じたステータス変数（IoTTx_Tx_Result）
	 * ============================================================= */
	IoTTx_Tx_Result transmitEx() override;

	enum FileExist {
		FE_NOT_EXIST,
		FE_EXIST,
		FE_FTP_ERROR
	};

	/*! ==============================================================
	 * @brief ファイルパスからディレクトリまでのパスを取得します
	 * @param[in] (path) ファイルパス
	 * @return ディレクトリまでのパス
	 * ============================================================= */
	String getDirName(String path);

	/*! ==============================================================
	 * @brief ファイルパスからファイル名を取得します
	 * @param[in] (path) ファイルパス
	 * @return ファイル名
	 * ============================================================= */
	String getFileName(String path);

	/*! ==============================================================
	 * @brief ファイルパスを結合します
	 * @param[in] (p1) パス1
	 * @param[in] (p2) パス2
	 * @return 結合されたパス
	 * ============================================================= */
	String joinPath(String p1, String p2);

	/*! ==============================================================
	 * @brief ディレクトリを再帰的に作成します
	 * @param[in] (absolute_path) 作成するディレクトリの絶対パス
	 * @return 作成成功の場合は true, 失敗の場合は false
	 * ============================================================= */
	bool makeDirRecursive(String absolute_path);

	/*! ==============================================================
	 * @brief ファイルが存在するか確認します
	 * @param[in] (fpath) ファイルパス
	 * @return 確認結果（FileExist 列挙型）
	 * ============================================================= */
	IoTTxFTP::FileExist exist(String fpath);

	Arduino_FTPClient _ftp;

	uint8_t _failed_counter       = 0;
	uint8_t _file_not_exist_count = 0;

	String _file_name;
	String _current_dir;
	String _heartbeat_current_dir;
	String _heartbeat_file_name;
};

#endif