#include "precomp.h"
#include <vector>


	// APIs for Init
	_FUNASRAPI FUNASR_HANDLE  FunASRInit(std::map<std::string, std::string>& model_path, int thread_num, ASR_TYPE type)
	{
		funasr::Model* mm = funasr::CreateModel(model_path, thread_num, type);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  FunASROnlineInit(FUNASR_HANDLE asr_hanlde, std::vector<int> chunk_size)
	{
		funasr::Model* mm = funasr::CreateModel(asr_hanlde, chunk_size);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  FsmnVadInit(std::map<std::string, std::string>& model_path, int thread_num)
	{
		funasr::VadModel* mm = funasr::CreateVadModel(model_path, thread_num);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  FsmnVadOnlineInit(FUNASR_HANDLE fsmnvad_handle)
	{
		funasr::VadModel* mm = funasr::CreateVadModel(fsmnvad_handle);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  CTTransformerInit(std::map<std::string, std::string>& model_path, int thread_num, PUNC_TYPE type)
	{
		funasr::PuncModel* mm = funasr::CreatePuncModel(model_path, thread_num, type);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  FunOfflineInit(std::map<std::string, std::string>& model_path, int thread_num)
	{
		funasr::OfflineStream* mm = funasr::CreateOfflineStream(model_path, thread_num);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE  FunTpassInit(std::map<std::string, std::string>& model_path, int thread_num)
	{
		funasr::TpassStream* mm = funasr::CreateTpassStream(model_path, thread_num);
		return mm;
	}

	_FUNASRAPI FUNASR_HANDLE FunTpassOnlineInit(FUNASR_HANDLE tpass_handle, std::vector<int> chunk_size)
	{
		return funasr::CreateTpassOnlineStream(tpass_handle, chunk_size);
	}

	// APIs for ASR Infer
	_FUNASRAPI FUNASR_RESULT FunASRInferBuffer(FUNASR_HANDLE handle, const char* sz_buf, int n_len, FUNASR_MODE mode, QM_CALLBACK fn_callback, bool input_finished, int sampling_rate, std::string wav_format)
	{
		funasr::Model* recog_obj = (funasr::Model*)handle;
		if (!recog_obj)
			return nullptr;

		funasr::Audio audio(recog_obj->GetAsrSampleRate(),1);
		if(wav_format == "pcm" || wav_format == "PCM"){
			if (!audio.LoadPcmwav(sz_buf, n_len, &sampling_rate))
				return nullptr;
		}else{
			if (!audio.FfmpegLoad(sz_buf, n_len))
				return nullptr;
		}

		float* buff;
		int len;
		int flag = 0;
		funasr::FUNASR_RECOG_RESULT* p_result = new funasr::FUNASR_RECOG_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
			return p_result;
		}
		int n_step = 0;
		int n_total = audio.GetQueueSize();
		while (audio.Fetch(buff, len, flag) > 0) {
			string msg = recog_obj->Forward(buff, len, input_finished);
			p_result->msg += msg;
			n_step++;
			if (fn_callback)
				fn_callback(n_step, n_total);
		}

		return p_result;
	}

	_FUNASRAPI FUNASR_RESULT FunASRInfer(FUNASR_HANDLE handle, const char* sz_filename, FUNASR_MODE mode, QM_CALLBACK fn_callback, int sampling_rate)
	{
		funasr::Model* recog_obj = (funasr::Model*)handle;
		if (!recog_obj)
			return nullptr;

		funasr::Audio audio(recog_obj->GetAsrSampleRate(),1);
		if(funasr::is_target_file(sz_filename, "wav")){
			int32_t sampling_rate_ = -1;
			if(!audio.LoadWav(sz_filename, &sampling_rate_))
				return nullptr;
		}else if(funasr::is_target_file(sz_filename, "pcm")){
			if (!audio.LoadPcmwav(sz_filename, &sampling_rate))
				return nullptr;
		}else{
			if (!audio.FfmpegLoad(sz_filename))
				return nullptr;
		}

		float* buff;
		int len;
		int flag = 0;
		int n_step = 0;
		int n_total = audio.GetQueueSize();
		funasr::FUNASR_RECOG_RESULT* p_result = new funasr::FUNASR_RECOG_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
            return p_result;
        }
		while (audio.Fetch(buff, len, flag) > 0) {
			string msg = recog_obj->Forward(buff, len, true);
			p_result->msg += msg;
			n_step++;
			if (fn_callback)
				fn_callback(n_step, n_total);
		}

		return p_result;
	}

	// APIs for VAD Infer
	_FUNASRAPI FUNASR_RESULT FsmnVadInferBuffer(FUNASR_HANDLE handle, const char* sz_buf, int n_len, QM_CALLBACK fn_callback, bool input_finished, int sampling_rate, std::string wav_format)
	{
		funasr::VadModel* vad_obj = (funasr::VadModel*)handle;
		if (!vad_obj)
			return nullptr;

		funasr::Audio audio(vad_obj->GetVadSampleRate(),1);
		if(wav_format == "pcm" || wav_format == "PCM"){
			if (!audio.LoadPcmwav(sz_buf, n_len, &sampling_rate))
				return nullptr;
		}else{
			if (!audio.FfmpegLoad(sz_buf, n_len))
				return nullptr;
		}

		funasr::FUNASR_VAD_RESULT* p_result = new funasr::FUNASR_VAD_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
			p_result->segments = new vector<std::vector<int>>();
            return p_result;
        }
		
		vector<std::vector<int>> vad_segments;
		audio.Split(vad_obj, vad_segments, input_finished);
		p_result->segments = new vector<std::vector<int>>(vad_segments);

		return p_result;
	}

	_FUNASRAPI FUNASR_RESULT FsmnVadInfer(FUNASR_HANDLE handle, const char* sz_filename, QM_CALLBACK fn_callback, int sampling_rate)
	{
		funasr::VadModel* vad_obj = (funasr::VadModel*)handle;
		if (!vad_obj)
			return nullptr;

		funasr::Audio audio(vad_obj->GetVadSampleRate(),1);
		if(funasr::is_target_file(sz_filename, "wav")){
			int32_t sampling_rate_ = -1;
			if(!audio.LoadWav(sz_filename, &sampling_rate_))
				return nullptr;
		}else if(funasr::is_target_file(sz_filename, "pcm")){
			if (!audio.LoadPcmwav(sz_filename, &sampling_rate))
				return nullptr;
		}else{
			if (!audio.FfmpegLoad(sz_filename))
				return nullptr;
		}

		funasr::FUNASR_VAD_RESULT* p_result = new funasr::FUNASR_VAD_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
			p_result->segments = new vector<std::vector<int>>();
            return p_result;
        }
		
		vector<std::vector<int>> vad_segments;
		audio.Split(vad_obj, vad_segments, true);
		p_result->segments = new vector<std::vector<int>>(vad_segments);

		return p_result;
	}

	// APIs for PUNC Infer
	_FUNASRAPI FUNASR_RESULT CTTransformerInfer(FUNASR_HANDLE handle, const char* sz_sentence, FUNASR_MODE mode, QM_CALLBACK fn_callback, PUNC_TYPE type, FUNASR_RESULT pre_result)
	{
		funasr::PuncModel* punc_obj = (funasr::PuncModel*)handle;
		if (!punc_obj)
			return nullptr;
		
		FUNASR_RESULT p_result = nullptr;
		if (type==PUNC_OFFLINE){
			p_result = (FUNASR_RESULT)new funasr::FUNASR_PUNC_RESULT;
			((funasr::FUNASR_PUNC_RESULT*)p_result)->msg = punc_obj->AddPunc(sz_sentence);
		}else if(type==PUNC_ONLINE){
			if (!pre_result)
				p_result = (FUNASR_RESULT)new funasr::FUNASR_PUNC_RESULT;
			else
				p_result = pre_result;
			((funasr::FUNASR_PUNC_RESULT*)p_result)->msg = punc_obj->AddPunc(sz_sentence, ((funasr::FUNASR_PUNC_RESULT*)p_result)->arr_cache);
		}else{
			LOG(ERROR) << "Wrong PUNC_TYPE";
			exit(-1);
		}

		return p_result;
	}

	// APIs for Offline-stream Infer
	_FUNASRAPI FUNASR_RESULT FunOfflineInferBuffer(FUNASR_HANDLE handle, const char* sz_buf, int n_len, 
												   FUNASR_MODE mode, QM_CALLBACK fn_callback, const std::vector<std::vector<float>> &hw_emb, 
												   int sampling_rate, std::string wav_format, bool itn, FUNASR_DEC_HANDLE dec_handle)
	{
		funasr::OfflineStream* offline_stream = (funasr::OfflineStream*)handle;
		if (!offline_stream)
			return nullptr;

		funasr::Audio audio(offline_stream->asr_handle->GetAsrSampleRate(),1);
		try{
			if(wav_format == "pcm" || wav_format == "PCM"){
				if (!audio.LoadPcmwav(sz_buf, n_len, &sampling_rate))
					return nullptr;
			}else{
				if (!audio.FfmpegLoad(sz_buf, n_len))
					return nullptr;
			}
		}catch (std::exception const &e)
		{
			LOG(ERROR)<<e.what();
			return nullptr;
		}

		funasr::FUNASR_RECOG_RESULT* p_result = new funasr::FUNASR_RECOG_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
            return p_result;
        }
		if(offline_stream->UseVad()){
			audio.Split(offline_stream);
		}

		float* buff;
		int len;
		int flag = 0;

		int n_step = 0;
		int n_total = audio.GetQueueSize();
		float start_time = 0.0;
		std::string cur_stamp = "[";
		std::string lang = (offline_stream->asr_handle)->GetLang();
		while (audio.Fetch(buff, len, flag, start_time) > 0) {
			// dec reset
			funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)dec_handle;
			if (wfst_decoder){
				wfst_decoder->StartUtterance();
			}
			string msg = (offline_stream->asr_handle)->Forward(buff, len, true, hw_emb, dec_handle);
			std::vector<std::string> msg_vec = funasr::split(msg, '|');
			if(msg_vec.size()==0){
				continue;
			}
			if(lang == "en-bpe" && p_result->msg != ""){
				p_result->msg += " ";
			}
			p_result->msg += msg_vec[0];
			//timestamp
			if(msg_vec.size() > 1){
				std::vector<std::string> msg_stamp = funasr::split(msg_vec[1], ',');
				for(int i=0; i<msg_stamp.size()-1; i+=2){
					float begin = std::stof(msg_stamp[i])+start_time;
					float end = std::stof(msg_stamp[i+1])+start_time;
					cur_stamp += "["+std::to_string((int)(1000*begin))+","+std::to_string((int)(1000*end))+"],";
				}
			}
			n_step++;
			if (fn_callback)
				fn_callback(n_step, n_total);
		}
		if(cur_stamp != "["){
			cur_stamp.erase(cur_stamp.length() - 1);
			p_result->stamp += cur_stamp + "]";
		}
		if(offline_stream->UsePunc()){
			string punc_res = (offline_stream->punc_handle)->AddPunc((p_result->msg).c_str(), lang);
			p_result->msg = punc_res;
		}
#if !defined(__APPLE__)
		if(offline_stream->UseITN() && itn){
			string msg_itn = offline_stream->itn_handle->Normalize(p_result->msg);
			if(!(p_result->stamp).empty()){
				std::string new_stamp = funasr::TimestampSmooth(p_result->msg, msg_itn, p_result->stamp);
				if(!new_stamp.empty()){
					p_result->stamp = new_stamp;
				}
			}			
			p_result->msg = msg_itn;
		}
#endif
		if (!(p_result->stamp).empty()){
			p_result->stamp_sents = funasr::TimestampSentence(p_result->msg, p_result->stamp);
		}
		return p_result;
	}

	_FUNASRAPI FUNASR_RESULT FunOfflineInfer(FUNASR_HANDLE handle, const char* sz_filename, FUNASR_MODE mode, QM_CALLBACK fn_callback, 
											 const std::vector<std::vector<float>> &hw_emb, int sampling_rate, bool itn, FUNASR_DEC_HANDLE dec_handle)
	{
		funasr::OfflineStream* offline_stream = (funasr::OfflineStream*)handle;
		if (!offline_stream)
			return nullptr;
		
		funasr::Audio audio((offline_stream->asr_handle)->GetAsrSampleRate(),1);
		try{
			if(funasr::is_target_file(sz_filename, "wav")){
				int32_t sampling_rate_ = -1;
				if(!audio.LoadWav(sz_filename, &sampling_rate_))
					return nullptr;
			}else if(funasr::is_target_file(sz_filename, "pcm")){
				if (!audio.LoadPcmwav(sz_filename, &sampling_rate))
					return nullptr;
			}else{
				if (!audio.FfmpegLoad(sz_filename))
					return nullptr;
			}
		}catch (std::exception const &e)
		{
			LOG(ERROR)<<e.what();
			return nullptr;
		}
		
		funasr::FUNASR_RECOG_RESULT* p_result = new funasr::FUNASR_RECOG_RESULT;
		p_result->snippet_time = audio.GetTimeLen();
		if(p_result->snippet_time == 0){
            return p_result;
        }
		if(offline_stream->UseVad()){
			audio.Split(offline_stream);
		}

		float* buff;
		int len;
		int flag = 0;
		int n_step = 0;
		int n_total = audio.GetQueueSize();
		float start_time = 0.0;
		std::string cur_stamp = "[";
		std::string lang = (offline_stream->asr_handle)->GetLang();
		while (audio.Fetch(buff, len, flag, start_time) > 0) {
			// dec reset
			funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)dec_handle;
			if (wfst_decoder){
				wfst_decoder->StartUtterance();
			}
			string msg = (offline_stream->asr_handle)->Forward(buff, len, true, hw_emb, dec_handle);
			std::vector<std::string> msg_vec = funasr::split(msg, '|');
			if(msg_vec.size()==0){
				continue;
			}
			if(lang == "en-bpe" && p_result->msg != ""){
				p_result->msg += " ";
			}
			p_result->msg += msg_vec[0];
			//timestamp
			if(msg_vec.size() > 1){
				std::vector<std::string> msg_stamp = funasr::split(msg_vec[1], ',');
				for(int i=0; i<msg_stamp.size()-1; i+=2){
					float begin = std::stof(msg_stamp[i])+start_time;
					float end = std::stof(msg_stamp[i+1])+start_time;
					cur_stamp += "["+std::to_string((int)(1000*begin))+","+std::to_string((int)(1000*end))+"],";
				}
			}

			n_step++;
			if (fn_callback)
				fn_callback(n_step, n_total);
		}
		if(cur_stamp != "["){
			cur_stamp.erase(cur_stamp.length() - 1);
			p_result->stamp += cur_stamp + "]";
		}
		if(offline_stream->UsePunc()){
			string punc_res = (offline_stream->punc_handle)->AddPunc((p_result->msg).c_str(), lang);
			p_result->msg = punc_res;
		}
#if !defined(__APPLE__)
		if(offline_stream->UseITN() && itn){
			string msg_itn = offline_stream->itn_handle->Normalize(p_result->msg);
			if(!(p_result->stamp).empty()){
				std::string new_stamp = funasr::TimestampSmooth(p_result->msg, msg_itn, p_result->stamp);
				if(!new_stamp.empty()){
					p_result->stamp = new_stamp;
				}
			}
			p_result->msg = msg_itn;
		}
#endif
		if (!(p_result->stamp).empty()){
			p_result->stamp_sents = funasr::TimestampSentence(p_result->msg, p_result->stamp);
		}
		return p_result;
	}

#if !defined(__APPLE__)
	_FUNASRAPI const std::vector<std::vector<float>> CompileHotwordEmbedding(FUNASR_HANDLE handle, std::string &hotwords, ASR_TYPE mode)
	{
		if (mode == ASR_OFFLINE){
			funasr::OfflineStream* offline_stream = (funasr::OfflineStream*)handle;
    		std::vector<std::vector<float>> emb;
			if (!offline_stream)
				return emb;
			return (offline_stream->asr_handle)->CompileHotwordEmbedding(hotwords);
		}
		else if (mode == ASR_TWO_PASS){
			funasr::TpassStream* tpass_stream = (funasr::TpassStream*)handle;
    		std::vector<std::vector<float>> emb;
			if (!tpass_stream)
				return emb;
			return (tpass_stream->asr_handle)->CompileHotwordEmbedding(hotwords);
		}
		else{
			LOG(ERROR) << "Not implement: Online model does not support Hotword yet!";
			std::vector<std::vector<float>> emb;
			return emb;
		}
		
	}
#endif

	// APIs for 2pass-stream Infer
	_FUNASRAPI FUNASR_RESULT FunTpassInferBuffer(FUNASR_HANDLE handle, FUNASR_HANDLE online_handle, const char* sz_buf, 
												 int n_len, std::vector<std::vector<std::string>> &punc_cache, bool input_finished, 
												 int sampling_rate, std::string wav_format, ASR_TYPE mode, 
												 const std::vector<std::vector<float>> &hw_emb, bool itn, FUNASR_DEC_HANDLE dec_handle)
	{
		funasr::TpassStream* tpass_stream = (funasr::TpassStream*)handle;
		funasr::TpassOnlineStream* tpass_online_stream = (funasr::TpassOnlineStream*)online_handle;
		if (!tpass_stream || !tpass_online_stream)
			return nullptr;
		
		funasr::VadModel* vad_online_handle = (tpass_online_stream->vad_online_handle).get();
		if (!vad_online_handle)
			return nullptr;

		funasr::Audio* audio = ((funasr::FsmnVadOnline*)vad_online_handle)->audio_handle.get();

		funasr::Model* asr_online_handle = (tpass_online_stream->asr_online_handle).get();
		if (!asr_online_handle)
			return nullptr;
		int chunk_len = ((funasr::ParaformerOnline*)asr_online_handle)->chunk_len;
		
		funasr::Model* asr_handle = (tpass_stream->asr_handle).get();
		if (!asr_handle)
			return nullptr;

		funasr::PuncModel* punc_online_handle = (tpass_stream->punc_online_handle).get();
		if (!punc_online_handle)
			return nullptr;

		if(wav_format == "pcm" || wav_format == "PCM"){
			if (!audio->LoadPcmwavOnline(sz_buf, n_len, &sampling_rate))
				return nullptr;
		}else{
			// if (!audio->FfmpegLoad(sz_buf, n_len))
			// 	return nullptr;
			LOG(ERROR) <<"Wrong wav_format: " << wav_format ;
			return nullptr;
		}

		funasr::FUNASR_RECOG_RESULT* p_result = new funasr::FUNASR_RECOG_RESULT;
		p_result->snippet_time = audio->GetTimeLen();
		
		audio->Split(vad_online_handle, chunk_len, input_finished, mode);

		funasr::AudioFrame* frame = NULL;
		while(audio->FetchChunck(frame) > 0){
			string msg = ((funasr::ParaformerOnline*)asr_online_handle)->Forward(frame->data, frame->len, frame->is_final);
			if(mode == ASR_ONLINE){
				((funasr::ParaformerOnline*)asr_online_handle)->online_res += msg;
				if(frame->is_final){
					string online_msg = ((funasr::ParaformerOnline*)asr_online_handle)->online_res;
					string msg_punc = punc_online_handle->AddPunc(online_msg.c_str(), punc_cache[0]);
					p_result->tpass_msg = msg_punc;
#if !defined(__APPLE__)
					// ITN
					if(tpass_stream->UseITN() && itn){
						string msg_itn = tpass_stream->itn_handle->Normalize(msg_punc);
						p_result->tpass_msg = msg_itn;
					}
#endif
					((funasr::ParaformerOnline*)asr_online_handle)->online_res = "";
					p_result->msg += msg;
				}else{
					p_result->msg += msg;
				}
			}else if(mode == ASR_TWO_PASS){
				p_result->msg += msg;
			}
			if(frame != NULL){
				delete frame;
				frame = NULL;
			}
		}

		// timestamp
		std::string cur_stamp = "[";		
		while(audio->FetchTpass(frame) > 0){
			// dec reset
			funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)dec_handle;
			if (wfst_decoder){
				wfst_decoder->StartUtterance();
			}
			string msg = ((funasr::Paraformer*)asr_handle)->Forward(frame->data, frame->len, frame->is_final, hw_emb, dec_handle);

			std::vector<std::string> msg_vec = funasr::split(msg, '|');  // split with timestamp
			if(msg_vec.size()==0){
				continue;
			}
			msg = msg_vec[0];
			//timestamp
			if(msg_vec.size() > 1){
				std::vector<std::string> msg_stamp = funasr::split(msg_vec[1], ',');
				for(int i=0; i<msg_stamp.size()-1; i+=2){
					float begin = std::stof(msg_stamp[i]) + float(frame->global_start)/1000.0;
					float end = std::stof(msg_stamp[i+1]) + float(frame->global_start)/1000.0;
					cur_stamp += "["+std::to_string((int)(1000*begin))+","+std::to_string((int)(1000*end))+"],";
				}
			}

			if(cur_stamp != "["){
				cur_stamp.erase(cur_stamp.length() - 1);
				p_result->stamp += cur_stamp + "]";
			}

			string msg_punc = punc_online_handle->AddPunc(msg.c_str(), punc_cache[1]);
			if(input_finished){
				msg_punc += "。";
			}
			p_result->tpass_msg = msg_punc;
#if !defined(__APPLE__)
			if(tpass_stream->UseITN() && itn){
				string msg_itn = tpass_stream->itn_handle->Normalize(msg_punc);
				// TimestampSmooth
				if(!(p_result->stamp).empty()){
					std::string new_stamp = funasr::TimestampSmooth(p_result->tpass_msg, msg_itn, p_result->stamp);
					if(!new_stamp.empty()){
						p_result->stamp = new_stamp;
					}
				}
				p_result->tpass_msg = msg_itn;
			}
#endif
			if (!(p_result->stamp).empty()){
				p_result->stamp_sents = funasr::TimestampSentence(p_result->tpass_msg, p_result->stamp);
			}
			if(frame != NULL){
				delete frame;
				frame = NULL;
			}
		}

		if(input_finished){
			audio->ResetIndex();
		}

		return p_result;
	}

	_FUNASRAPI const int FunASRGetRetNumber(FUNASR_RESULT result)
	{
		if (!result)
			return 0;
		return 1;
	}

	// APIs for GetRetSnippetTime
	_FUNASRAPI const float FunASRGetRetSnippetTime(FUNASR_RESULT result)
	{
		if (!result)
			return 0.0f;

		return ((funasr::FUNASR_RECOG_RESULT*)result)->snippet_time;
	}

	_FUNASRAPI const float FsmnVadGetRetSnippetTime(FUNASR_RESULT result)
	{
		if (!result)
			return 0.0f;

		return ((funasr::FUNASR_VAD_RESULT*)result)->snippet_time;
	}

	// APIs for GetResult
	_FUNASRAPI const char* FunASRGetResult(FUNASR_RESULT result,int n_index)
	{
		funasr::FUNASR_RECOG_RESULT * p_result = (funasr::FUNASR_RECOG_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->msg.c_str();
	}

	_FUNASRAPI const char* FunASRGetStamp(FUNASR_RESULT result)
	{
		funasr::FUNASR_RECOG_RESULT * p_result = (funasr::FUNASR_RECOG_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->stamp.c_str();
	}

		_FUNASRAPI const char* FunASRGetStampSents(FUNASR_RESULT result)
	{
		funasr::FUNASR_RECOG_RESULT * p_result = (funasr::FUNASR_RECOG_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->stamp_sents.c_str();
	}

	_FUNASRAPI const char* FunASRGetTpassResult(FUNASR_RESULT result,int n_index)
	{
		funasr::FUNASR_RECOG_RESULT * p_result = (funasr::FUNASR_RECOG_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->tpass_msg.c_str();
	}

	_FUNASRAPI const char* CTTransformerGetResult(FUNASR_RESULT result,int n_index)
	{
		funasr::FUNASR_PUNC_RESULT * p_result = (funasr::FUNASR_PUNC_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->msg.c_str();
	}

	_FUNASRAPI vector<std::vector<int>>* FsmnVadGetResult(FUNASR_RESULT result,int n_index)
	{
		funasr::FUNASR_VAD_RESULT * p_result = (funasr::FUNASR_VAD_RESULT*)result;
		if(!p_result)
			return nullptr;

		return p_result->segments;
	}

	// APIs for FreeResult
	_FUNASRAPI void FunASRFreeResult(FUNASR_RESULT result)
	{
		if (result)
		{
			delete (funasr::FUNASR_RECOG_RESULT*)result;
		}
	}

	_FUNASRAPI void CTTransformerFreeResult(FUNASR_RESULT result)
	{
		if (result)
		{
			delete (funasr::FUNASR_PUNC_RESULT*)result;
		}
	}

	_FUNASRAPI void FsmnVadFreeResult(FUNASR_RESULT result)
	{
		funasr::FUNASR_VAD_RESULT * p_result = (funasr::FUNASR_VAD_RESULT*)result;
		if (p_result)
		{
			if(p_result->segments){
				delete p_result->segments;
			}
			delete p_result;
		}
	}

        // APIs for decoder status reset
	_FUNASRAPI void FunASRReset(FUNASR_HANDLE handle, FUNASR_DEC_HANDLE dec_handle)
	{
		funasr::Model* recog_obj = (funasr::Model*)handle;
		recog_obj->StartUtterance();
		funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)dec_handle;
		if (wfst_decoder)
			wfst_decoder->StartUtterance();
	}

	_FUNASRAPI void FunOfflineReset(FUNASR_HANDLE handle, FUNASR_DEC_HANDLE dec_handle)
	{
		funasr::OfflineStream* recog_obj = (funasr::OfflineStream*)handle;
		recog_obj->asr_handle->StartUtterance();
		funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)dec_handle;
		if (wfst_decoder)
			wfst_decoder->StartUtterance();
	}

	// APIs for Uninit
	_FUNASRAPI void FunASRUninit(FUNASR_HANDLE handle)
	{
		funasr::Model* recog_obj = (funasr::Model*)handle;

		if (!recog_obj)
			return;

		delete recog_obj;
	}

	_FUNASRAPI void FsmnVadUninit(FUNASR_HANDLE handle)
	{
		funasr::VadModel* recog_obj = (funasr::VadModel*)handle;

		if (!recog_obj)
			return;

		delete recog_obj;
	}

	_FUNASRAPI void CTTransformerUninit(FUNASR_HANDLE handle)
	{
		funasr::PuncModel* punc_obj = (funasr::PuncModel*)handle;

		if (!punc_obj)
			return;

		delete punc_obj;
	}

	_FUNASRAPI void FunOfflineUninit(FUNASR_HANDLE handle)
	{
		funasr::OfflineStream* offline_stream = (funasr::OfflineStream*)handle;

		if (!offline_stream)
			return;

		delete offline_stream;
	}

	_FUNASRAPI void FunTpassUninit(FUNASR_HANDLE handle)
	{
		funasr::TpassStream* tpass_stream = (funasr::TpassStream*)handle;

		if (!tpass_stream)
			return;

		delete tpass_stream;
	}

	_FUNASRAPI void FunTpassOnlineUninit(FUNASR_HANDLE handle)
	{
		funasr::TpassOnlineStream* tpass_online_stream = (funasr::TpassOnlineStream*)handle;

		if (!tpass_online_stream)
			return;

		delete tpass_online_stream;
	}

	_FUNASRAPI FUNASR_DEC_HANDLE FunASRWfstDecoderInit(FUNASR_HANDLE handle, int asr_type, float glob_beam, float lat_beam, float am_scale)
	{
		funasr::WfstDecoder* mm = nullptr;
		if (asr_type == ASR_OFFLINE) {
			funasr::OfflineStream* offline_stream = (funasr::OfflineStream*)handle;
			funasr::Paraformer* paraformer = (funasr::Paraformer*)offline_stream->asr_handle.get();
			if (paraformer->lm_)
				mm = new funasr::WfstDecoder(paraformer->lm_.get(),
					paraformer->GetPhoneSet(), paraformer->GetLmVocab(), glob_beam, lat_beam, am_scale);
		} else if (asr_type == ASR_TWO_PASS){
			funasr::TpassStream* tpass_stream = (funasr::TpassStream*)handle;
			funasr::Paraformer* paraformer = (funasr::Paraformer*)tpass_stream->asr_handle.get();
			if (paraformer->lm_)
				mm = new funasr::WfstDecoder(paraformer->lm_.get(), 
					paraformer->GetPhoneSet(), paraformer->GetLmVocab(), glob_beam, lat_beam, am_scale);
		}
		return mm;
	}

	_FUNASRAPI void FunASRWfstDecoderUninit(FUNASR_DEC_HANDLE handle)
	{
		funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)handle;
		if (!wfst_decoder)
			return;
		delete wfst_decoder;
	}

	_FUNASRAPI void FunWfstDecoderLoadHwsRes(FUNASR_DEC_HANDLE handle, int inc_bias, unordered_map<string, int> &hws_map)
	{
		funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)handle;
		if (!wfst_decoder)
			return;
		wfst_decoder->LoadHwsRes(inc_bias, hws_map);
	}
	_FUNASRAPI void FunWfstDecoderUnloadHwsRes(FUNASR_DEC_HANDLE handle)
	{
		funasr::WfstDecoder* wfst_decoder = (funasr::WfstDecoder*)handle;
		if (!wfst_decoder)
			return;
		wfst_decoder->UnloadHwsRes();
	}
